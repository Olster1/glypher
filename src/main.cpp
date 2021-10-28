#include "string.h"
#include "easy_types.h"
#include "easy_font.h"

/*Parameters:

1. font file name (.ttf)
2. start code point 
3. end code point
3. output file name
4. json or binary output (json binary) (default json)

*/



int main(int argc, char *args[]) {
    
    if(argc >= 5) {
        
        char *fileName = args[1];
        u32 startCodePoint = args[2]; //ttf file
        u32 endCodePoint = args[3];
        u32 outputFileName = args[4];
        u32 outputFileType = args[5];
        
        easyFont_createSDFFont(fileName, startCodePoint, endCodePoint);
        
        easyFont_loadFontAtlas(concatInArena(globalExeBasePath, fontName, &globalPerFrameArena), &globalLongTermArena);
    } else {
        printf("please pass parameters");
    }
    
    return(0);
}
