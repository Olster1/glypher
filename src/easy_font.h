/*
File to enable writing with fonts easy to add to your project. Uses Sean Barret's stb_truetype file to load and write the fonts. Requires a current openGL context. 
*/

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

///////////////////////************ New API *************////////////////////
static void easyFont_createSDFFont(char *fileName, u32 startCodePoint, u32 endCodePoint) {
    
    FileContents contents = platformReadEntireFile(fileName, false);
    
    //NOTE(ollie): This stores all the info about the font
    stbtt_fontinfo *fontInfo = pushStruct(&globalLongTermArena, stbtt_fontinfo);
    
    //NOTE(ollie): Fill out the font info
    stbtt_InitFont(fontInfo, contents.memory, 0);
    
    //NOTE(ollie): Get the 'scale' for the max pixel height 
    float maxHeightForFontInPixels = 32;//pixels
    float scale = stbtt_ScaleForPixelHeight(fontInfo, maxHeightForFontInPixels);
    
    //NOTE(ollie): Scale the padding around the glyph proportional to the size of the glyph
    s32 padding = (s32)(maxHeightForFontInPixels / 3);
    //NOTE(ollie): The distance from the glyph center that determines the edge (i.e. all the 'in' pixels)
    u8 onedge_value = (u8)(0.8f*255); 
    //NOTE(ollie): The rate at which the distance from the center should increase
    float pixel_dist_scale = (float)onedge_value/(float)padding;
    
    InfiniteAlloc atlasElms = initInfinteAlloc(Easy_AtlasElm);
    
    char *fontName = getFileLastPortionWithoutExtension_arena(fileName, &globalPerFrameArena);
    
    ///////////////////////************ Kerning Table *************////////////////////
    // //NOTE(ollie): Get the kerning table length i.e. number of entries
    // int  kerningTableLength = stbtt_GetKerningTableLength(fontInfo);
    
    // //NOTE(ollie): Allocate kerning table
    // stbtt_kerningentry* kerningTable = pushArray(&globalPerFrameArena, kerningTableLength, stbtt_kerningentry);
    
    // //NOTE(ollie): Fill out the table
    // stbtt_GetKerningTable(fontInfo, kerningTable, kerningTableLength);
    
    for(s32 codeIndex = startCodePoint; codeIndex <= endCodePoint; ++codeIndex) {
        
        int width;
        int height;
        int xoffset; 
        int yoffset;
        
        u8 *sdfBitmap = stbtt_GetCodepointSDF(fontInfo, scale, codeIndex, padding, onedge_value, pixel_dist_scale, &width, &height, &xoffset, &yoffset);    
        
        if(width > 0 && height > 0) {
            Easy_AtlasElm elm = {};
            
            //NOTE(ollie): Make sure names are in a memory arena, not on the stack
            char buffer[512];
            sprintf(buffer, "_%d", codeIndex);
            elm.shortName = concatInArena(fontName, buffer, &globalPerFrameArena);
            elm.longName = concatInArena(fileName, buffer, &globalPerFrameArena);
            
            elm.tex = {};
            elm.tex.width = width;
            elm.tex.height = height;
            elm.tex.aspectRatio_h_over_w = height / width;
            elm.tex.uvCoords = rect2f(0, 0, 1, 1);
            
            //NOTE(ollie): Set the font specific information
            elm.xOffset = xoffset;
            elm.yOffset = yoffset;
            elm.codepoint = codeIndex;
            elm.fontHeight = maxHeightForFontInPixels + (2*padding);
            elm.hasTexture = true;
            /////
            
            if(sdfBitmap) {
                //NOTE(ollie): Take memory mark before we push new bitmap on 
                perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);
                
                //NOTE(ollie): Blow out bitmap to 32bits per pixel instead of 8 so it's easier to load to the GPU
                u32 *sdfBitmap_32 = (u32 *)pushSize(&globalPerFrameArena, width*height*sizeof(u32));
                for(int y = 0; y < height; ++y) {
                    for(int x = 0; x < width; ++x) {
                        u8 alpha = sdfBitmap[y*width + x];
                        if(alpha != 0) {
                            int j = 0;
                        }
                        sdfBitmap_32[y*width + x] = 0x00000000 | (u32)(((u32)alpha) << 24);
                    }
                }
                ////////////////////////////////////////////////////////////////////
                
                //NOTE(ollie): Load the texture to the GPU
                elm.tex.id = renderLoadTexture(width, height, sdfBitmap_32, RENDER_TEXTURE_DEFAULT);
                
                //NOTE(ollie): Release memory from 32bit bitmap
                releaseMemoryMark(&perFrameArenaMark);
            } else {
                elm.tex.id = globalWhiteTexture.id;
                elm.hasTexture = false;
            }
            
            //NOTE(ollie): Add to the array
            addElementInifinteAllocWithCount_(&atlasElms, &elm, 1);
        }
        
        //NOTE(ollie): Free the bitmap data
        stbtt_FreeSDF(sdfBitmap, 0);
    }
    
    //NOTE(ollie): Sort the elements by size first to get better layout
    easyAtlas_sortBySize(&atlasElms);
    
    
    //NOTE(ollie): Starting dimensions
    EasyAtlas_Dimensions dimensions = {};
    
    //NOTE(ollie): Start at 512 & move down. It would be better to do a binary search between 0-4096 to find the best size
    float rectX = 1028;
    float rectY = 1028;
    
    float increment = 32;
    
    char *outputFileName = concatInArena(concatInArena(globalExeBasePath, "./fontAtlas_", &globalPerFrameArena), fontName, &globalPerFrameArena);
    
    // NOTE(ollie): Try find the smallest size to fint everyone in
    do {
        dimensions = easyAtlas_drawAtlas(outputFileName, &globalPerFrameArena, &atlasElms, false, 5, rectX, rectY, EASY_ATLAS_FONT_ATLAS, 1);
        easyAtlas_refreshAllElements(&atlasElms);
        rectX -= increment;
        rectY -= increment;
    } while(dimensions.count == 1);
    
    rectX += 2*increment;
    rectY += 2*increment;
    
    easyAtlas_refreshAllElements(&atlasElms);
    //NOTE(ollie): Actually draw the atlas to disk now
    easyAtlas_drawAtlas(outputFileName, &globalPerFrameArena, &atlasElms, true, 5, rectX, rectY, EASY_ATLAS_FONT_ATLAS, 1);
    
    ///////////////////////*********** Cleanup the memory used**************////////////////////
    
    for(int index = 0; index < atlasElms.count; ++index) {
        Easy_AtlasElm *atlasElm = (Easy_AtlasElm *)getElementFromAlloc_(&atlasElms, index);
        if(atlasElm->tex.id > 0 && atlasElm->tex.id != globalWhiteTexture.id) {
            //NOTE(ollie): Delete all the textures we allocated
            renderDeleteTexture(&atlasElm->tex);
        }
    }
    
    //NOTE(ollie): Release the memory were using
    releaseInfiniteAlloc(&atlasElms);
    
    /*
    Bitmap functions

    stbtt_MakeCodepointBitmap()         
    stbtt_GetCodepointBitmapBox()
    */
}

typedef struct {
    Texture texture;
    
    s32 xOffset;
    s32 yOffset;
    
    u32 codepoint;
    
    //NOTE(ollie): Whether it has an actual texture or is just empty whitespace like 'space'
    bool hasTexture;    
    
} EasyFont_Glyph;

#define EASY_MAX_GLYPHS_PER_FONT 4096


typedef struct {
    u32 glyphCount;
    EasyFont_Glyph glyphs[EASY_MAX_GLYPHS_PER_FONT];
    
    s32 fontHeight;
} EasyFont_Font;

static EasyFont_Font *easyFont_loadFontAtlas(char *fileName, Arena *arena) {
    
    EasyFont_Font *font = pushStruct(arena, EasyFont_Font); 
    
    bool stillLoading = true;
    
    //NOTE(ollie): Atlas start at 1 for the loop index, so we want to mimic that
    u32 countAt = 1; 
    while(stillLoading) {
        
        char *buffer0 = easy_createString_printf(&globalPerFrameArena, "%s_%d.txt", fileName, countAt);
        
        if(!platformDoesFileExist(buffer0)) {
            //NOTE(ollie): No more atlas files to load
            stillLoading = false;
        } else {
            
            char *buffer1 = easy_createString_printf(&globalPerFrameArena, "%s_%d.png", fileName, countAt);
            
            bool premultiplyAlpha = false;
            Texture atlasTex = loadImage(buffer1, TEXTURE_FILTER_LINEAR, false, premultiplyAlpha);
            
            FileContents contentsText = getFileContentsNullTerminate(buffer0);
            assert(contentsText.valid);
            
            EasyTokenizer tokenizer = lexBeginParsing((char *)contentsText.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);
            bool parsing = true;
            
            Rect2f uvCoords = rect2f(0, 0, 0, 0);
            int imgWidth = 0;
            int imgHeight = 0;
            s32 codepoint = 0;
            s32 xoffset = 0;
            s32 yoffset = 0;
            bool hasTexture = true;
            
            while(parsing) {
                EasyToken token = lexGetNextToken(&tokenizer);
                switch(token.type) {
                    case TOKEN_NULL_TERMINATOR: {
                        parsing = false;
                    } break;
                    case TOKEN_CLOSE_BRACKET: {
                        if(font->glyphCount < arrayCount(font->glyphs)) {
                            EasyFont_Glyph *g = font->glyphs + font->glyphCount++;
                            
                            g->texture.id = atlasTex.id;
                            g->texture.width = imgWidth;
                            g->texture.height = imgHeight;
                            g->texture.uvCoords = uvCoords;
                            g->texture.aspectRatio_h_over_w = easyRender_getTextureAspectRatio_HOverW(&g->texture);
                            g->texture.name = "glyph";
                            
                            g->codepoint = codepoint;
                            g->xOffset = xoffset;
                            g->yOffset = yoffset;
                            g->hasTexture = hasTexture;
                        }                
                        
                    } break;
                    case TOKEN_WORD: {
                        if(stringsMatchNullN("width", token.at, token.size)) {
                            imgWidth = getIntFromDataObjects(&tokenizer);
                        }
                        if(stringsMatchNullN("height", token.at, token.size)) {
                            imgHeight = getIntFromDataObjects(&tokenizer);
                        }
                        if(stringsMatchNullN("fontHeight", token.at, token.size)) {
                            font->fontHeight = getIntFromDataObjects(&tokenizer);
                        }
                        if(stringsMatchNullN("hasTexture", token.at, token.size)) {
                            hasTexture = (bool)getIntFromDataObjects(&tokenizer);
                        }
                        if(stringsMatchNullN("uvCoords", token.at, token.size)) {
                            V4 uv = buildV4FromDataObjects(&tokenizer);
                            //copy over to make a rect4 instead of a V4
                            uvCoords.E[0] = uv.E[0];
                            uvCoords.E[1] = uv.E[1];
                            uvCoords.E[2] = uv.E[2];
                            uvCoords.E[3] = uv.E[3];
                        }
                        if(stringsMatchNullN("xoffset", token.at, token.size)) {
                            xoffset = getIntFromDataObjects(&tokenizer);
                        }
                        if(stringsMatchNullN("yoffset", token.at, token.size)) {
                            yoffset = getIntFromDataObjects(&tokenizer);
                        }
                        if(stringsMatchNullN("codepoint", token.at, token.size)) {
                            codepoint = getIntFromDataObjects(&tokenizer);
                        }
                    } break;
                    default: {
                        
                    }
                }
            }
            
            releaseInfiniteAlloc(&tokenizer.typesArray);
            
            easyFile_endFileContents(&contentsText);
        }
        //NOTE(ollie): Increment the loop count
        countAt++;
    }
    
    return font;
}


static inline EasyFont_Glyph *easyFont_findGlyph(EasyFont_Font *font, u32 unicodePoint) {
    DEBUG_TIME_BLOCK()
        //NOTE(ollie): @speed 
        EasyFont_Glyph *result = 0;
    for(int i = 0; i < font->glyphCount && !result; ++i) {
        EasyFont_Glyph *glyph = font->glyphs + i;
        
        if(glyph->codepoint == unicodePoint) {
            result = glyph;
            break;
        }
        
    }
    return result;
}