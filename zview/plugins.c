#include "general.h"
#include "ztext.h"
#include "plugins.h"
#include "pic_load.h"
#include "plugins/common/zvplugin.h"
#include "plugins/common/plugver.h"



/* Global variable */
int16_t plugins_nbr;
int16_t encoder_plugins_nbr;
CODEC *codecs[MAX_CODECS];


/*==================================================================================*
 * void plugins_quit:																*
 *		unload all the codec from memory.											*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		--																			*
 *----------------------------------------------------------------------------------*
 * returns: 																		*
 *      --																			*
 *==================================================================================*/
static void plugin_free(CODEC *codec, boolean waitpid)
{
	switch (codec->type)
	{
	case CODEC_LDG:
		ldg_close(codec->c.ldg, ldg_global);
		break;
	case CODEC_SLB:
		plugin_close(&codec->c.slb, waitpid);
		break;
	}
	free(codec);
}

void plugins_quit( void)
{
	int16 i;

	for( i = 0; i < plugins_nbr; i++)
	{
		plugin_free(codecs[i], FALSE);
	}
	plugins_nbr = 0;
}


static boolean warn_duplicates(int16_t i, CODEC *last_codec, const char *ext, CODEC *this_codec)
{
	char buf[256];
	int ret;

	sprintf(buf, get_string(AL_DUPLICATE), ext, last_codec->name, this_codec->name);
	ret = FormAlert(1, buf);
	switch (ret)
	{
	case 1:
		/* replace previous by current */
		plugin_free(codecs[i], TRUE);
		codecs[i] = last_codec;
		return TRUE;
	case 2:
		/* ignore current */
		plugin_free(last_codec, TRUE);
		return TRUE;
	}
	return FALSE;
}


static boolean check_duplicates(void)
{
	int16_t last = plugins_nbr;
	int16_t i;
	CODEC *last_codec = codecs[last];
	CODEC *this_codec;
	const char *p1;
	const char *p2;
	
	for (i = 0; i < last; i++)
	{
		this_codec = codecs[i];
		for (p1 = last_codec->extensions; *p1; p1 += strlen(p1) + 1)
		{
			for (p2 = this_codec->extensions; *p2; p2 += strlen(p2) + 1)
			{
				if (strcmp(p1, p2) == 0)
				{
					return warn_duplicates(i, last_codec, p1, this_codec);
				}
			}
		}
	}

	plugins_nbr++;
	return TRUE;
}


static void copy_extensions(char *dst, const char *src, int num_extensions, int ext_size)
{
	if (num_extensions == 0)
	{
		/*
		 * newer plugin, with 0-terminated list
		 */
		size_t elen;

		while (*src != '\0')
		{
			elen = strlen(src) + 1;
			strcpy(dst, src);
			src += elen;
			dst += elen;
		}
		*dst = '\0';
	} else
	{
		int i, j;
		
		for (i = 0; i < num_extensions; i++)
		{
			for (j = 0; j < ext_size; j++)
			{
				if (*src != '\0')
				{
					if (*src != ' ')
						*dst++ = *src;
				}
				src++;
			}
			*dst++ = '\0';
		}
		*dst = '\0';
	}
}


static size_t calc_extension_len(const char *extensions, int num_extensions, int ext_size)
{
	if (num_extensions == 0)
	{
		/*
		 * newer plugin, with 0-terminated list
		 */
		size_t len = 1;
		size_t elen;
		const char *p = extensions;

		while (*p != '\0')
		{
			elen = strlen(p) + 1;
			len += elen;
			p += elen;
		}
		return len;
	}
	/* old version, with exactly 3 chars per extension */
	return num_extensions * (ext_size + 1) + 1;
}


/*==================================================================================*
 * void plugins_init:																*
 *		load all the codec to memory.												*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		--																			*
 *----------------------------------------------------------------------------------*
 * returns: 																		*
 *      --																			*
 *==================================================================================*/
int16 plugins_init( void)
{
	char plugin_dir[MAX_PATH];
	DIR	 			*dir;
	LDG_INFOS 		*cook = 0;
	struct dirent	*de;
	size_t			namelen;
	char  			extension[4];
	char *name;
	CODEC *codec;
	int16_t i, j;
	
	strcpy( plugin_dir, zview_path);
	strcat( plugin_dir, "codecs\\");
	
	/* We try to find the zcodecs folder in zview directory... 	*/
	if ((dir = opendir(plugin_dir)) == NULL)
	{
		char 			*env_ldg = 0;

		shel_envrn( &env_ldg, "LDGPATH=");

		/* ...if this directory doesn't exist, we try to find 
	   	   the zcodecs folder in the default LDG path, with 
	   	   the LDG cookie or with the environment variable.	*/

		if( ldg_cookie( LDG_COOKIE, ( int32*)&cook) && cook)
			strcpy(plugin_dir, cook->path);
		else if( env_ldg)
			strcpy( plugin_dir, env_ldg);
		else 
			strcpy( plugin_dir, "C:\\gemsys\\ldg\\");

		namelen = strlen(plugin_dir);
		
		if( namelen > 0 && plugin_dir[namelen-1] != '\\' && plugin_dir[namelen-1] != '/')
			strcat( plugin_dir, "\\"); 

		strcat( plugin_dir, "codecs\\");

		dir = opendir(plugin_dir);
	}

	if (dir != NULL)
	{
		name = plugin_dir + strlen(plugin_dir);
		while ((de = readdir(dir)) != NULL && plugins_nbr < MAX_CODECS)
		{
			namelen = strlen(de->d_name);
			if (namelen < 3)
				continue;
			strcpy(name, de->d_name);
			strcpy(extension, de->d_name + namelen - 3);
			str2lower(extension);

			if (strcmp(extension, "ldg") == 0)
			{
				LDG *ldg;
				
				if ((ldg = ldg_open(plugin_dir, ldg_global)) != NULL)
				{
					void CDECL(*codec_init)(void);
					size_t extension_len;
					const char *extensions;
					char *dst;
					int num_extensions;
					
					if ((codec_init = ldg_find("plugin_init", ldg)) != NULL)
					{
						extensions = ldg->infos;
						num_extensions = ldg->user_ext;
						extension_len = calc_extension_len(extensions, num_extensions, 3);
						codec = codecs[plugins_nbr] = (CODEC *)calloc(1, sizeof(CODEC) + extension_len + namelen + 1);
						if (codec == NULL)
						{
							errshow(NULL, -ENOMEM);
							break;
						}
						codec->type = CODEC_LDG;
						codec->c.ldg = ldg;
						dst = (char *)(codec + 1);
						codec->extensions = dst;
						copy_extensions(dst, extensions, num_extensions, 3);
						dst += extension_len;
						codec->name = dst;
						strcpy(dst, de->d_name);
						codec->capabilities = 0;
						if (ldg_find("reader_init", ldg) != 0)
							codec->capabilities |= CAN_DECODE;
						if (ldg_find("encoder_init", ldg) != 0)
							codec->capabilities |= CAN_ENCODE;
						if (!check_duplicates())
							break;
						codec_init();
					} else
					{
						errshow(de->d_name, LDG_ERR_BASE + ldg_error());
						ldg_close(ldg, ldg_global);
					}
				} else
				{
					errshow(de->d_name, LDG_ERR_BASE + ldg_error());
				}
			} else if (strcmp(extension, "slb") == 0)
			{
				SLB slb;
				long err;
				size_t extension_len;
				const char *extensions;
				char *dst;
				
				*name = '\0';
				slb.handle = 0;
				if ((err = plugin_open(de->d_name, plugin_dir, &slb)) >= 0)
				{
					extensions = (const char *)plugin_get_option(&slb, OPTION_EXTENSIONS);
					extension_len = calc_extension_len(extensions, 0, 0);
					codec = codecs[plugins_nbr] = (CODEC *)calloc(1, sizeof(CODEC) + extension_len + namelen + 1);
					if (codec == NULL)
					{
						errshow(NULL, -ENOMEM);
						break;
					}
					codec->type = CODEC_SLB;
					codec->c.slb = slb;
					dst = (char *)(codec + 1);
					codec->extensions = dst;
					copy_extensions(dst, extensions, 0, 0);
					dst += extension_len;
					codec->name = dst;
					strcpy(dst, de->d_name);
					codec->capabilities = plugin_get_option(&slb, OPTION_CAPABILITIES);
					if (codec->capabilities < 0)
						codec->capabilities = 0;
					if (!check_duplicates())
						break;
				} else
				{
					if (err == -EINVAL)
						err = PLUGIN_MISMATCH;
					errshow(de->d_name, err);
				}
			}
		}
		closedir(dir);
	}

	/*
	 * now sort the encoders first, so we can use the same array when saving
	 */
	j = 0;
	for (i = 0; i < plugins_nbr; i++)
	{
		if (codecs[i]->capabilities & CAN_ENCODE)
		{
			CODEC *tmp = codecs[i];
			codecs[i] = codecs[j];
			codecs[j] = tmp;
			j++;
		}
	}
	encoder_plugins_nbr = j;

	return plugins_nbr;
}
