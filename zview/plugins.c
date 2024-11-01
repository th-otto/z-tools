#include "general.h"
#include "ztext.h"
#include "plugins.h"
#include "pic_load.h"
#include "plugins/common/zvplugin.h"
#include "plugins/common/plugver.h"
#define NF_DEBUG 1
#if NF_DEBUG
#include <mint/arch/nf_ops.h>
#else
#define nf_debugprintf(format, ...)
#endif


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
		ldg_close(codec->c.ldg.ldg, ldg_global);
		codec->c.ldg.ldg = NULL;
		break;
	case CODEC_SLB:
		plugin_close(&codec->c.slb, waitpid);
		break;
	}
	free(codec);
}

void plugins_quit(void)
{
	int16 i;

	for (i = 0; i < plugins_nbr; i++)
	{
		plugin_free(codecs[i], FALSE);
	}
	plugins_nbr = 0;
}


static void fill_ldg_funcs(LDG *ldg, struct _ldg_funcs *funcs)
{
	funcs->decoder_init = ldg_find("reader_init", ldg);
	funcs->decoder_read = ldg_find("reader_read", ldg);
	funcs->decoder_quit = ldg_find("reader_quit", ldg);
	funcs->decoder_get_txt = ldg_find("reader_get_txt", ldg);
	funcs->encoder_init = ldg_find("encoder_init", ldg);
	funcs->encoder_write = ldg_find("encoder_write", ldg);
	funcs->encoder_quit = ldg_find("encoder_quit", ldg);
	funcs->set_jpg_option = ldg_find("set_jpg_option", ldg);
	funcs->set_tiff_option = ldg_find("set_tiff_option", ldg);
	funcs->set_webp_option = ldg_find("set_webp_option", ldg);
	/* funcs->decoder_get_page_size = ldg_find("reader_get_page_size", ldg); */
}


boolean plugin_ref(CODEC *codec)
{
	long err;

	if (!codec)
		return FALSE;
	if (codec->refcount == 0)
	{
		switch (codec->type)
		{
		case CODEC_LDG:
			{
				void CDECL(*codec_init)(void);
				char plugin_name[MAX_PATH];

				strcpy(plugin_name, zview_plugin_path);
				strcat(plugin_name, codec->name);
				if ((codec->c.ldg.ldg = ldg_open(plugin_name, ldg_global)) == NULL ||
					(codec_init = ldg_find("plugin_init", codec->c.ldg.ldg)) == NULL)
				{
					errshow(codec->name, LDG_ERR_BASE + ldg_error());
					return FALSE;
				}
				fill_ldg_funcs(codec->c.ldg.ldg, codec->c.ldg.funcs);
				codec_init();
			}
			break;
		case CODEC_SLB:
			codec->c.slb.handle = 0;
			if ((err = plugin_open(codec->name, zview_plugin_path, &codec->c.slb)) < 0)
			{
				if (err == -EINVAL)
					err = PLUGIN_MISMATCH;
				errshow(codec->name, err);
				return FALSE;
			}
			break;
		default:
			return FALSE;
		}		
		nf_debugprintf("plugin_ref(%s): loaded\n", codec->name);
	}
	if (codec->state & CODEC_RESIDENT)
	{
		/* only incremented once, because we dont unref if */
		if (codec->refcount == 0)
			++codec->refcount;
	} else if (++codec->refcount == 0)
	{
		/* counter overflowed */
		nf_debugprintf("plugin_ref(%s): overflow\n", codec->name);
		codec->refcount = -1;
	}
	nf_debugprintf("plugin_ref(%s): refcount=%u%s\n", codec->name, codec->refcount, (codec->state & CODEC_RESIDENT) ? " (resident)" : "");

	return TRUE;
}


void plugin_unref(CODEC *codec)
{
	if (!codec)
		return;
	if (codec->state & CODEC_RESIDENT)
	{
		nf_debugprintf("plugin_unref(%s): refcount=%u (resident)\n", codec->name, codec->refcount);
		return;
	}
	if (codec->refcount == 0)
	{
		/* counter underflowed */
		nf_debugprintf("plugin_unref(%s): underflow\n", codec->name);
		return;
	}
	if (--codec->refcount == 0)
	{
		nf_debugprintf("plugin_unref(%s): unloading\n", codec->name);
		switch (codec->type)
		{
		case CODEC_LDG:
			ldg_close(codec->c.ldg.ldg, ldg_global);
			codec->c.ldg.ldg = NULL;
			/* note: function pointers are also invalid now */
			break;
		case CODEC_SLB:
			plugin_close(&codec->c.slb, FALSE);
			break;
		}
	}
	nf_debugprintf("plugin_unref(%s): refcount=%u\n", codec->name, codec->refcount);
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
int16 plugins_init(void)
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
	
	strcpy(plugin_dir, zview_path);
	strcat(plugin_dir, "codecs\\");
	
	/* We try to find the zcodecs folder in zview directory... 	*/
	if ((dir = opendir(plugin_dir)) == NULL)
	{
		char 			*env_ldg = 0;

		shel_envrn(&env_ldg, "LDGPATH=");

		/* ...if this directory doesn't exist, we try to find 
	   	   the zcodecs folder in the default LDG path, with 
	   	   the LDG cookie or with the environment variable.	*/

		if (ldg_cookie(LDG_COOKIE, (int32*)&cook) && cook)
			strcpy(plugin_dir, cook->path);
		else if (env_ldg)
			strcpy(plugin_dir, env_ldg);
		else 
			strcpy(plugin_dir, "C:\\gemsys\\ldg\\");

		namelen = strlen(plugin_dir);
		
		if (namelen > 0 && plugin_dir[namelen-1] != '\\' && plugin_dir[namelen-1] != '/')
			strcat(plugin_dir, "\\"); 

		strcat(plugin_dir, "codecs\\");

		dir = opendir(plugin_dir);
	}

	if (dir != NULL)
	{
		strcpy(zview_plugin_path, plugin_dir);
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
					size_t extra_size;
					const char *extensions;
					char *dst;
					int num_extensions;
					
					if ((codec_init = ldg_find("plugin_init", ldg)) != NULL)
					{
						extensions = ldg->infos;
						num_extensions = ldg->user_ext;
						extension_len = calc_extension_len(extensions, num_extensions, 3);
						extra_size = sizeof(struct _ldg_funcs) + extension_len + namelen + 1;
						codec = codecs[plugins_nbr] = (CODEC *)calloc(1, sizeof(CODEC) + extra_size);
						if (codec == NULL)
						{
							errshow(NULL, -ENOMEM);
							break;
						}
						codec->type = CODEC_LDG;
						codec->c.ldg.ldg = ldg;
						codec->refcount = 1;
						dst = (char *)(codec + 1);
						codec->c.ldg.funcs = (struct _ldg_funcs *)dst;
						dst += sizeof(struct _ldg_funcs);
						codec->extensions = dst;
						copy_extensions(dst, extensions, num_extensions, 3);
						dst += extension_len;
						codec->name = dst;
						strcpy(dst, de->d_name);
						codec->capabilities = 0;
						fill_ldg_funcs(codec->c.ldg.ldg, codec->c.ldg.funcs);
						if (codec->c.ldg.funcs->decoder_init != 0)
							codec->capabilities |= CAN_DECODE;
						if (codec->c.ldg.funcs->encoder_init != 0)
							codec->capabilities |= CAN_ENCODE;
						if (!check_duplicates())
							break;
						codec_init();
						/*
						 * make GOD plugin resident, because it is needed soon for the icons
						 */
						if (strcmp(extensions, "GOD") == 0)
							codec->refcount++;
						/*
 						 * now that we have all the meta information, we can unload it again
						 */
						plugin_unref(codec);
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
				size_t extra_size;
				const char *extensions;
				char *dst;
				
				*name = '\0';
				slb.handle = 0;
				if ((err = plugin_open(de->d_name, plugin_dir, &slb)) >= 0)
				{
					extensions = (const char *)plugin_get_option(&slb, OPTION_EXTENSIONS);
					extension_len = calc_extension_len(extensions, 0, 0);
					extra_size = extension_len + namelen + 1;
					codec = codecs[plugins_nbr] = (CODEC *)calloc(1, sizeof(CODEC) + extra_size);
					if (codec == NULL)
					{
						errshow(NULL, -ENOMEM);
						break;
					}
					codec->type = CODEC_SLB;
					codec->c.slb = slb;
					codec->refcount = 1;
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
					/*
					 * make GOD plugin resident, because it is needed soon for the icons
					 */
					if (strcmp(extensions, "GOD") == 0)
						codec->refcount++;
					/*
					 * now that we have all the meta information, we can unload it again
					 */
					plugin_unref(codec);
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
	 * now sort the encoders first.
	 * The save_dialog relies on the encoder plugins being first.
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
