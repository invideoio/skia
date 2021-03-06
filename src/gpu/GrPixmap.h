/*
 * Copyright 2020 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrPixmap_DEFINED
#define GrPixmap_DEFINED

#include "include/core/SkPixmap.h"
#include "src/gpu/GrImageInfo.h"

class GrPixmap {
public:
    GrPixmap() = default;
    GrPixmap(const GrPixmap&) = default;
    GrPixmap(GrPixmap&&) = default;
    GrPixmap& operator=(const GrPixmap&) = default;
    GrPixmap& operator=(GrPixmap&&) = default;

    GrPixmap(GrImageInfo info, void* addr, size_t rowBytes)
            : fAddr(addr), fRowBytes(rowBytes), fInfo(std::move(info)) {
        if (fRowBytes < info.minRowBytes() || !addr) {
            *this = {};
        }
    }
    /* implicit */ GrPixmap(const SkPixmap& pixmap)
            : GrPixmap(pixmap.info(), pixmap.writable_addr(), pixmap.rowBytes()) {}

    const GrImageInfo& info() const { return fInfo; }
    const GrColorInfo& colorInfo() const { return fInfo.colorInfo(); }

    void* addr() const { return fAddr; }
    size_t rowBytes() const { return fRowBytes; }

    bool hasPixels() const { return SkToBool(fAddr); }

    int width() const { return fInfo.width(); }
    int height() const { return fInfo.height(); }
    SkISize dimensions() const { return fInfo.dimensions(); }
    GrColorType colorType() const { return fInfo.colorType(); }
    SkAlphaType alphaType() const { return fInfo.alphaType(); }

    /**
     * Map this pixmap to a rect in a surface of indicated dimensions at offset surfacePt. Clip the
     * logical rectangle to the bounds of the surface. If the rect does not intersect the surface
     * bounds or is empty then return a default GrPixmap. Otherwise, surfacePt is updated to refer
     * to the upper left of the clipped rectangle. The returned pixmap will refer to the portion
     * of the original pixmap inside the surface bounds.
     */
    GrPixmap clip(SkISize surfaceDims, SkIPoint* surfacePt) {
        auto bounds = SkIRect::MakeSize(surfaceDims);
        auto rect = SkIRect::MakePtSize(*surfacePt, this->dimensions());
        if (!rect.intersect(bounds)) {
            return {};
        }
        void* addr = static_cast<char*>(fAddr) + (rect.fTop  - surfacePt->fY)*fRowBytes +
                                                 (rect.fLeft - surfacePt->fX)*fInfo.bpp();
        surfacePt->fX = rect.fLeft;
        surfacePt->fY = rect.fTop;
        return {this->info().makeDimensions(rect.size()), addr, fRowBytes};
    }

    /** Returns a GrPixmap and a unique_ptr that owns the storage backing the pixmap. */
    static std::tuple<GrPixmap, std::unique_ptr<char[]>> Allocate(const GrImageInfo& info) {
        size_t rb = info.minRowBytes();
        size_t size = info.height()*rb;
        if (!size) {
            return {};
        }
        std::unique_ptr<char[]> storage(new char[size]);
        return {GrPixmap(info, storage.get(), rb), std::move(storage)};
    }

private:
    void* fAddr = nullptr;
    size_t fRowBytes = 0;
    GrImageInfo fInfo;
};

#endif
