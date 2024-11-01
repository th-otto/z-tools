#ifndef __ZVIEW_PLUGINS_H__
#define __ZVIEW_PLUGINS_H__

#include <mint/slb.h>

struct _img_info;

struct _ldg_funcs {
	boolean	__CDECL ( *decoder_init)	(const char *, struct _img_info *);
	void 	__CDECL ( *decoder_quit)	(struct _img_info *);
	boolean	__CDECL ( *decoder_read)	(struct _img_info *, uint8 *);
	void	__CDECL ( *decoder_get_txt)	(struct _img_info *, txt_data *);
	void 	__CDECL ( *encoder_quit)	(struct _img_info *);
	boolean	__CDECL ( *encoder_write)	(struct _img_info *, uint8 *);
	boolean	__CDECL ( *encoder_init)	(const char *, struct _img_info *);
	void 	__CDECL ( *set_jpg_option)  (int16 set_quality, int /* J_COLOR_SPACE */ set_color_space, boolean set_progressive);
	void 	__CDECL ( *set_tiff_option) (int16 set_quality, uint16 set_encode_compression);
	void 	__CDECL ( *set_webp_option) (int16 set_quality, uint16 set_compression_level);
};

typedef struct {
	int type;
#define CODEC_NONE     0  /* empty slot */
#define CODEC_LDG      1  /* old LDG style codec */
#define CODEC_SLB      2  /* new SLB style codec */
	unsigned int state;
/*
 * bitmasks for codec.state
 */
#define CODEC_DECODER_INITIALIZED   0x0001
#define CODEC_ENCODER_INITIALIZED   0x0002
#define CODEC_RESIDENT              0x0004
	unsigned int refcount;
	union {
		struct {
			LDG *ldg;
			struct _ldg_funcs *funcs;
		} ldg;
		SLB slb;
	} c;
	long capabilities;
	const char *name;
	const char *extensions;
} CODEC;
#define MAX_CODECS 200


extern CODEC *codecs[MAX_CODECS];
extern int16 plugins_nbr;
extern int16 encoder_plugins_nbr;

extern CODEC *curr_input_plugin;
extern CODEC *curr_output_plugin;

int16 plugins_init(void);
void plugins_quit(void);
boolean plugin_ref(CODEC *codec);
void plugin_unref(CODEC *codec);

#endif /* __ZVIEW_PLUGINS_H__ */
