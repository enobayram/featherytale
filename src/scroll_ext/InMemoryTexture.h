/*
 * InMemoryTexture.h
 *
 *  Created on: Oct 26, 2015
 *      Author: eba
 */

#ifndef SCROLL_EXT_INMEMORYTEXTURE_H_
#define SCROLL_EXT_INMEMORYTEXTURE_H_

#include <vector>
#include <functional>

#include <orx.h>

class InMemoryTexture {
    std::vector<orxU32> data;
    int w,h;
public:
    InMemoryTexture():w(0), h(0) {}
    InMemoryTexture(std::vector<orxU32> data, uint w, uint h): data(std::move(data)), w(w), h(h) {}
    orxU32 get(int x, int y) {
        orxASSERT(x<w && y < h);
        return data[y*w + x];
    }
    bool inTexture(int x, int y) {
        return x>0 && x<w && y>0 && y<h;
    }
};

void init_in_memory_texture();

void get_texture(const char * imagename, std::function<void(InMemoryTexture)>);

orxSTATUS orxFASTCALL inMemoryTextureLoadHandler(const orxEVENT *_pstEvent);

#endif /* SCROLL_EXT_INMEMORYTEXTURE_H_ */
