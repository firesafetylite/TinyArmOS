#include "../../third_party/console-font/font8x19.h"
#define NATIVE_TEXT_COLUMNS 80U
#define NATIVE_TEXT_ROWS 25U
_Static_assert(NATIVE_TEXT_ROWS * NATIVE_FONT_HEIGHT <= 480U, "Text must fit the framebuffer");
static UINTN native_text_top(void) { return (native_api->height - NATIVE_TEXT_ROWS * NATIVE_FONT_HEIGHT) / 2U; }
static const UINT32 native_palette[16]={0x000000,0x0000aa,0x00aa00,0x00aaaa,0xaa0000,0xaa00aa,0xaa5500,0xaaaaaa,
    0x555555,0x5555ff,0x55ff55,0x55ffff,0xff5555,0xff55ff,0xffff55,0xffffff};
static void native_graphics_clear(void) {
    if (!native_api->framebuffer) return;
    UINT32 color=native_palette[(native_mode.Attribute>>4)&7];
    for (UINTN i=0;i<(UINTN)native_api->width*native_api->height;i++) native_api->framebuffer[i]=color;
}
static void native_graphics_scroll(void) {
    if (!native_api->framebuffer) return;
    UINTN width=native_api->width;
    UINT32 *pixels=native_api->framebuffer + native_text_top()*width;
    for (UINTN i=0;i<width*(NATIVE_TEXT_ROWS-1U)*NATIVE_FONT_HEIGHT;i++) pixels[i]=pixels[i+width*NATIVE_FONT_HEIGHT];
    UINT32 background=native_palette[(native_mode.Attribute>>4)&7];
    for (UINTN i=width*(NATIVE_TEXT_ROWS-1U)*NATIVE_FONT_HEIGHT;i<width*NATIVE_TEXT_ROWS*NATIVE_FONT_HEIGHT;i++) pixels[i]=background;
}
static void native_console_character(CHAR16 ch) {
    if (ch=='\r') { native_mode.CursorColumn=0; return; }
    if (ch==8) { if (native_mode.CursorColumn) native_mode.CursorColumn--; return; }
    if (ch=='\n') native_mode.CursorRow++;
    else {
        if (native_api->framebuffer) {
            UINTN x=(UINTN)native_mode.CursorColumn*NATIVE_FONT_WIDTH;
            UINTN y=native_text_top()+(UINTN)native_mode.CursorRow*NATIVE_FONT_HEIGHT;
            UINT32 foreground=native_palette[native_mode.Attribute&15], background=native_palette[(native_mode.Attribute>>4)&7];
            if (ch<32 || ch>126) ch='?';
            for (UINTN row=0;row<NATIVE_FONT_HEIGHT;row++) for (UINTN column=0;column<NATIVE_FONT_WIDTH;column++)
                native_api->framebuffer[(y+row)*native_api->width+x+column]=(native_font[ch-32][row]&(0x80U>>column)) ? foreground : background;
        }
        if (++native_mode.CursorColumn>=(INT32)NATIVE_TEXT_COLUMNS) { native_mode.CursorColumn=0; native_mode.CursorRow++; }
    }
    if (native_mode.CursorRow>=(INT32)NATIVE_TEXT_ROWS) { native_mode.CursorRow=NATIVE_TEXT_ROWS-1U; native_graphics_scroll(); }
}
static EFI_GRAPHICS_OUTPUT_MODE_INFORMATION native_gop_info;
static EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE native_gop_mode;
static EFI_GRAPHICS_OUTPUT_PROTOCOL native_gop;
static EFI_STATUS native_gop_query(EFI_GRAPHICS_OUTPUT_PROTOCOL *self, UINT32 mode, UINTN *bytes, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **info) {
    (void)self; if (mode || !native_api->framebuffer) return EFI_UNSUPPORTED;
    EFI_STATUS result=native_allocate(2,sizeof(native_gop_info),(void **)info);
    if (result!=EFI_SUCCESS) return result;
    **info=native_gop_info; *bytes=sizeof(native_gop_info); return EFI_SUCCESS;
}
static EFI_STATUS native_gop_set(EFI_GRAPHICS_OUTPUT_PROTOCOL *self, UINT32 mode) {
    (void)self; return mode || !native_api->framebuffer ? EFI_UNSUPPORTED : EFI_SUCCESS;
}
static EFI_STATUS native_gop_blt(EFI_GRAPHICS_OUTPUT_PROTOCOL *self, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *buffer, UINT32 operation,
    UINTN source_x, UINTN source_y, UINTN destination_x, UINTN destination_y, UINTN width, UINTN height, UINTN delta) {
    (void)self;
    if (!buffer || !native_api->framebuffer || !width || !height || destination_x>=native_api->width || destination_y>=native_api->height ||
        width>native_api->width-destination_x || height>native_api->height-destination_y || (operation!=0 && operation!=2)) return EFI_UNSUPPORTED;
    if (!delta) delta=width*sizeof(*buffer);
    if (operation==2 && (source_x+width<width || source_x+width>delta/sizeof(*buffer) || source_y+height<height ||
        source_y+height>(~0ULL)/delta)) return EFI_ACCESS_DENIED;
    for (UINTN y=0;y<height;y++) for (UINTN x=0;x<width;x++) {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pixel=operation==0 ? buffer : (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((UINT8 *)buffer+(source_y+y)*delta)+source_x+x;
        native_api->framebuffer[(destination_y+y)*native_api->width+destination_x+x]=((UINT32)pixel->Red<<16)|((UINT32)pixel->Green<<8)|pixel->Blue;
    }
    return EFI_SUCCESS;
}
static void native_graphics_initialize(void) {
    native_gop_info=(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION){0,native_api->width,native_api->height,1,{0,0,0,0},native_api->width};
    native_gop_mode=(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE){1,0,&native_gop_info,sizeof(native_gop_info),(UINTN)native_api->framebuffer,
        (UINTN)native_api->width*native_api->height*4};
    native_gop=(EFI_GRAPHICS_OUTPUT_PROTOCOL){native_gop_query,native_gop_set,native_gop_blt,&native_gop_mode};
}
