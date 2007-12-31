/*****************************************************************************
 * font.h
 *****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
extern void init_font(void);
void write_font_init_GX(GXColor fontColor);
void write_font_color(GXColor* fontColorPtr);
void write_font(int x, int y, char *string, float scale);
void write_font_centered(int y, char *string, float scale);
void write_font_origin(char *string, float scale);

#ifdef __cplusplus
}
#endif
