#include <DirectXTex.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <stdexcept>
#include <vector>

#define CHECK(expression)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expression))                                                                                             \
            throw std::runtime_error(#expression);                                                                     \
    } while (false)
using namespace DirectX;
namespace
{
    std::ofstream snapshot;
    void Record(const ScratchImage& image)
    {
        if (!snapshot.is_open())
            return;
        for (size_t i = 0; i < image.GetImageCount(); ++i)
        {
            const auto& entry = image.GetImages()[i];
            const uint64_t fields[]{entry.width, entry.height, static_cast<uint64_t>(entry.format), entry.rowPitch};
            snapshot.write(reinterpret_cast<const char*>(fields), sizeof(fields));
            snapshot.write(reinterpret_cast<const char*>(entry.pixels), static_cast<std::streamsize>(entry.slicePitch));
        }
    }
    ScratchImage ColorImage(size_t width, size_t height, float r = .25f, float g = .5f, float b = .75f, float a = 1.f)
    {
        ScratchImage image;
        CHECK(SUCCEEDED(image.Initialize2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, 1, 1)));
        auto* pixels = reinterpret_cast<XMFLOAT4*>(image.GetPixels());
        std::fill_n(pixels, width * height, XMFLOAT4(r, g, b, a));
        return image;
    }
    void Equal(const ScratchImage& a, const ScratchImage& b)
    {
        CHECK(a.GetImageCount() == b.GetImageCount());
        CHECK(a.GetMetadata().format == b.GetMetadata().format);
        CHECK(a.GetMetadata().mipLevels == b.GetMetadata().mipLevels);
        CHECK(a.GetMetadata().arraySize == b.GetMetadata().arraySize);
        CHECK(a.GetMetadata().depth == b.GetMetadata().depth);
        for (size_t i = 0; i < a.GetImageCount(); ++i)
        {
            const auto& x = a.GetImages()[i];
            const auto& y = b.GetImages()[i];
            CHECK(x.width == y.width && x.height == y.height && x.slicePitch == y.slicePitch);
            CHECK(std::memcmp(x.pixels, y.pixels, x.slicePitch) == 0);
        }
    }
    void KnownBlocks()
    {
        // Independent constant-color blocks catch channel, alpha and signedness mistakes.
        std::array<uint8_t, 16> block{};
        block[1] = 0xf8;
        Image source{1, 1, DXGI_FORMAT_BC1_UNORM, 8, 8, block.data()};
        ScratchImage decoded;
        CHECK(SUCCEEDED(Decompress(source, DXGI_FORMAT_R8G8B8A8_UNORM, decoded)));
        const uint8_t red[]{255, 0, 0, 255};
        CHECK(std::memcmp(decoded.GetPixels(), red, 4) == 0);
        Record(decoded);
        block.fill(0);
        std::fill(block.begin() + 4, block.begin() + 8, 255);
        CHECK(SUCCEEDED(Decompress(source, DXGI_FORMAT_R8G8B8A8_UNORM, decoded)));
        CHECK(decoded.GetPixels()[3] == 0);
        Record(decoded);
        block.fill(0);
        std::fill_n(block.begin(), 8, 0xaa);
        block[9] = 0xf8;
        source   = {1, 1, DXGI_FORMAT_BC2_UNORM, 16, 16, block.data()};
        CHECK(SUCCEEDED(Decompress(source, DXGI_FORMAT_R8G8B8A8_UNORM, decoded)));
        CHECK(decoded.GetPixels()[0] == 255 && decoded.GetPixels()[3] == 170);
        Record(decoded);
        block.fill(0);
        block[0]      = 128;
        block[1]      = 64;
        block[9]      = 0xf8;
        source.format = DXGI_FORMAT_BC3_UNORM;
        CHECK(SUCCEEDED(Decompress(source, DXGI_FORMAT_R8G8B8A8_UNORM, decoded)));
        CHECK(decoded.GetPixels()[0] == 255 && decoded.GetPixels()[3] == 128);
        Record(decoded);
        source   = {1, 1, DXGI_FORMAT_BC4_SNORM, 8, 8, block.data()};
        block[0] = 129;
        CHECK(SUCCEEDED(Decompress(source, DXGI_FORMAT_R32G32B32_FLOAT, decoded)));
        CHECK(reinterpret_cast<float*>(decoded.GetPixels())[0] == -1.f);
        Record(decoded);
    }
    void BCFormats()
    {
        constexpr DXGI_FORMAT formats[]{
            DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB, DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB,
            DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM_SRGB, DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC4_SNORM,
            DXGI_FORMAT_BC5_UNORM, DXGI_FORMAT_BC5_SNORM,      DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_SF16,
            DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM_SRGB};
        for (auto format : formats)
        {
            for (auto dimensions : {std::pair<size_t, size_t>{1, 1}, {2, 3}, {5, 7}})
            {
                auto input = ColorImage(dimensions.first, dimensions.second);
                ScratchImage compressed, decoded, loaded;
                CHECK(SUCCEEDED(Compress(*input.GetImage(0, 0, 0), format, TEX_COMPRESS_BC7_QUICK, .5f, compressed)));
                CHECK(SUCCEEDED(Decompress(compressed.GetImages(), compressed.GetImageCount(), compressed.GetMetadata(),
                                           DXGI_FORMAT_UNKNOWN, decoded)));
                CHECK(decoded.GetImage(0, 0, 0)->width == dimensions.first);
                CHECK(decoded.GetImage(0, 0, 0)->height == dimensions.second);
                Blob blob;
                CHECK(SUCCEEDED(SaveToDDSMemory(compressed.GetImages(), compressed.GetImageCount(),
                                                compressed.GetMetadata(), DDS_FLAGS_NONE, blob)));
                CHECK(SUCCEEDED(
                    LoadFromDDSMemory(blob.GetBufferPointer(), blob.GetBufferSize(), DDS_FLAGS_NONE, nullptr, loaded)));
                Equal(compressed, loaded);
                Record(compressed);
                Record(decoded);
            }
        }
        for (auto format : {DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_SF16})
        {
            const float red = format == DXGI_FORMAT_BC6H_SF16 ? -2.f : 4.f;
            auto input      = ColorImage(4, 4, red, 2.f, 1.f);
            ScratchImage bc, decoded;
            CHECK(SUCCEEDED(Compress(*input.GetImage(0, 0, 0), format, TEX_COMPRESS_DEFAULT, .5f, bc)));
            CHECK(SUCCEEDED(Decompress(*bc.GetImage(0, 0, 0), DXGI_FORMAT_R32G32B32_FLOAT, decoded)));
            CHECK(std::abs(reinterpret_cast<float*>(decoded.GetPixels())[0] - red) < .1f);
            Record(decoded);
        }
    }

    void Containers()
    {
        for (int kind = 0; kind < 4; ++kind)
        {
            ScratchImage image, loaded;
            if (kind == 0)
                CHECK(SUCCEEDED(image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 8, 4, 2, 3)));
            if (kind == 1)
                CHECK(SUCCEEDED(image.InitializeCube(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4, 2, 3)));
            if (kind == 2)
                CHECK(SUCCEEDED(image.Initialize3D(DXGI_FORMAT_R8G8B8A8_UNORM, 8, 4, 4, 3)));
            if (kind == 3)
                CHECK(SUCCEEDED(image.Initialize1D(DXGI_FORMAT_R8_UNORM, 8, 2, 3)));
            for (size_t i = 0; i < image.GetPixelsSize(); ++i)
                image.GetPixels()[i] = static_cast<uint8_t>(i * 17);
            Blob blob;
            CHECK(SUCCEEDED(
                SaveToDDSMemory(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DDS_FLAGS_NONE, blob)));
            TexMetadata metadata{};
            CHECK(SUCCEEDED(
                GetMetadataFromDDSMemory(blob.GetBufferPointer(), blob.GetBufferSize(), DDS_FLAGS_NONE, metadata)));
            CHECK(metadata.IsCubemap() == (kind == 1));
            CHECK(SUCCEEDED(
                LoadFromDDSMemory(blob.GetBufferPointer(), blob.GetBufferSize(), DDS_FLAGS_NONE, nullptr, loaded)));
            Equal(image, loaded);
            Record(loaded);
            CHECK(FAILED(LoadFromDDSMemory(blob.GetBufferPointer(), 127, DDS_FLAGS_NONE, nullptr, loaded)));
            CHECK(FAILED(
                LoadFromDDSMemory(blob.GetBufferPointer(), blob.GetBufferSize() - 1, DDS_FLAGS_NONE, nullptr, loaded)));
            auto* bytes = static_cast<uint8_t*>(blob.GetBufferPointer());
            std::fill_n(bytes + 12, 8, 0xff);  // impossible width and height
            CHECK(FAILED(LoadFromDDSMemory(bytes, blob.GetBufferSize(), DDS_FLAGS_NONE, nullptr, loaded)));
        }
        ScratchImage empty;
        CHECK(FAILED(LoadFromDDSMemory(static_cast<const uint8_t*>(nullptr), 0, DDS_FLAGS_NONE, nullptr, empty)));
    }
    void UpstreamValidation()
    {
        size_t levels = 0;
        CHECK(CalculateMipLevels(9, 3, levels) && levels == 4);
        levels = 5;
        CHECK(!CalculateMipLevels(9, 3, levels));
        levels = 0;
        CHECK(CalculateMipLevels3D(4, 2, 16, levels) && levels == 5);
        levels = 6;
        CHECK(!CalculateMipLevels3D(4, 2, 16, levels));

        auto input = ColorImage(2, 2, .5f, .5f, .5f, .5f);
        ScratchImage converted;
        for (const auto format : {DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_A8_UNORM})
        {
            CHECK(SUCCEEDED(Convert(*input.GetImages(), format, TEX_FILTER_FORCE_NON_WIC, .5f, converted)));
            CHECK(std::all_of(converted.GetPixels(), converted.GetPixels() + converted.GetPixelsSize(),
                              [](uint8_t value) { return value == 128; }));
            Record(converted);
        }
        auto invalid   = *input.GetImages();
        invalid.format = DXGI_FORMAT_UNKNOWN;
        CHECK(Convert(invalid, DXGI_FORMAT_R8G8B8A8_UNORM, TEX_FILTER_DEFAULT, .5f, converted) == E_INVALIDARG);
        CHECK(Compress(invalid, DXGI_FORMAT_BC1_UNORM, TEX_COMPRESS_DEFAULT, .5f, converted) == E_INVALIDARG);
        CHECK(Resize(invalid, 1, 1, TEX_FILTER_DEFAULT, converted) == E_INVALIDARG);
        CHECK(ComputeNormalMap(invalid, CNMAP_CHANNEL_RED, 1.f, DXGI_FORMAT_R8G8B8A8_UNORM, converted) == E_INVALIDARG);
        CHECK(ConvertToSinglePlane(input.GetImages(), input.GetImageCount(), input.GetMetadata(), converted) ==
              E_INVALIDARG);

        Blob blob;
        CHECK(SUCCEEDED(SaveToDDSMemory(*input.GetImages(), DDS_FLAGS_FORCE_DX10_EXT, blob)));
        // DX10 resourceDimension at byte 132: permissive mode recognizes zero as 2D.
        auto* bytes = blob.GetBufferPointer();
        std::fill_n(bytes + 132, 4, 0);
        CHECK(FAILED(LoadFromDDSMemory(bytes, blob.GetBufferSize(), DDS_FLAGS_NONE, nullptr, converted)));
        CHECK(SUCCEEDED(LoadFromDDSMemory(bytes, blob.GetBufferSize(), DDS_FLAGS_PERMISSIVE, nullptr, converted)));
        Equal(input, converted);
        Record(converted);

        auto metadata      = input.GetMetadata();
        metadata.miscFlags = TEX_MISC_TEXTURECUBE;
        metadata.arraySize = 7;
        std::array<uint8_t, 148> header{};
        size_t required = 0;
        CHECK(EncodeDDSHeader(metadata, DDS_FLAGS_FORCE_DX10_EXT, header.data(), header.size(), required) ==
              E_INVALIDARG);
    }

    void PitchBounds()
    {
        size_t row = 0, slice = 0;
        CHECK(SUCCEEDED(ComputePitch(DXGI_FORMAT_BC1_UNORM, 5, 7, row, slice)) && row == 16 && slice == 32);
        CHECK(SUCCEEDED(ComputePitch(DXGI_FORMAT_NV12, 5, 6, row, slice)) && row == 6 && slice == 54);
        CHECK(SUCCEEDED(ComputePitch(DXGI_FORMAT_R8_UNORM, 5, 3, row, slice, CP_FLAGS_LEGACY_DWORD)) && row == 8 &&
              slice == 24);
        constexpr size_t maximum = (std::numeric_limits<size_t>::max)();
        for (const auto format :
             {DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_YUY2, DXGI_FORMAT_R32G32B32A32_FLOAT})
        {
            row = slice = 123;
            CHECK(FAILED(ComputePitch(format, maximum, maximum, row, slice)) && row == 0 && slice == 0);
        }
#ifndef LIBDDS_REFERENCE
        // Upstream's multiplication guards do not cover alignment and planar additions.
        for (const auto alignment :
             {CP_FLAGS_NONE, CP_FLAGS_LEGACY_DWORD, CP_FLAGS_PARAGRAPH, CP_FLAGS_YMM, CP_FLAGS_ZMM, CP_FLAGS_PAGE4K})
        {
            CHECK(FAILED(ComputePitch(DXGI_FORMAT_R1_UNORM, maximum, 1, row, slice, alignment)) && row == 0 &&
                  slice == 0);
        }
        if constexpr (sizeof(size_t) == 8)
        {
            CHECK(FAILED(ComputePitch(DXGI_FORMAT_R8_UNORM, maximum / 8, 1, row, slice, CP_FLAGS_LEGACY_DWORD)) &&
                  row == 0 && slice == 0);
            for (const auto format : {DXGI_FORMAT_NV12, DXGI_FORMAT_P010})
            {
                CHECK(FAILED(ComputePitch(format, 2, size_t(0xaaaaaaaaaaaaaaacULL), row, slice)) && row == 0 &&
                      slice == 0);
            }
            CHECK(FAILED(ComputePitch(DXGI_FORMAT_V208, 1, size_t(1ULL << 63), row, slice)) && row == 0 && slice == 0);
            CHECK(FAILED(ComputePitch(DXGI_FORMAT_V408, 1, size_t(0x5555555555555556ULL), row, slice)) && row == 0 &&
                  slice == 0);
        }
#endif
    }

    void HDRBounds()
    {
        const std::string prefix = "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n";
        const std::string header = prefix + "-Y 1 +X 8\n";
        std::vector<uint8_t> valid(header.begin(), header.end());
        valid.insert(valid.end(), {2, 2, 0, 8, 136, 128, 136, 64, 136, 32, 136, 129});
        ScratchImage decoded;
        CHECK(SUCCEEDED(LoadFromHDRMemory(valid.data(), valid.size(), nullptr, decoded)));
        Record(decoded);
        for (size_t size = 0; size < valid.size(); ++size)
        {
            // A separate exact-sized allocation makes truncated-header reads visible to ASan.
            const std::vector<uint8_t> truncated(valid.begin(), valid.begin() + size);
            CHECK(FAILED(LoadFromHDRMemory(truncated.data(), truncated.size(), nullptr, decoded)));
        }
        for (const auto orientation : {"+Y 1 +X 8\n", "-X 1 +X 8\n"})
        {
            const auto wrongHeader = prefix + orientation;
            std::vector<uint8_t> wrong(wrongHeader.begin(), wrongHeader.end());
            wrong.insert(wrong.end(), valid.begin() + header.size(), valid.end());
            CHECK(FAILED(LoadFromHDRMemory(wrong.data(), wrong.size(), nullptr, decoded)));
        }
        std::vector<uint8_t> literal(header.begin(), header.end());
        literal.insert(literal.end(), {2, 2, 0, 8, 8, 128, 64});
        CHECK(FAILED(LoadFromHDRMemory(literal.data(), literal.size(), nullptr, decoded)));
    }

    void DDS24Bit()
    {
        const auto path = (std::filesystem::current_path() / "libdds-24bpp.dds").wstring();
        for (int kind = 0; kind < 7; ++kind)
        {
#ifdef LIBDDS_REFERENCE
            // The untouched reference writes a mismatched 24bpp payload for implicit DX10 arrays.
            if (kind >= 5)
                continue;
#endif
            ScratchImage input, loaded, fromFile;
            if (kind == 1)
                CHECK(SUCCEEDED(input.Initialize3D(DXGI_FORMAT_B8G8R8X8_UNORM, 5, 3, 2, 3)));
            else if (kind == 2 || kind == 6)
                CHECK(SUCCEEDED(input.InitializeCube(DXGI_FORMAT_B8G8R8X8_UNORM, 4, 4, kind == 6 ? 2 : 1, 3)));
            else
                CHECK(SUCCEEDED(input.Initialize2D(DXGI_FORMAT_B8G8R8X8_UNORM, 5, 3, kind == 5 ? 2 : 1, 3)));
            for (size_t i = 0; i < input.GetPixelsSize(); ++i)
                input.GetPixels()[i] = (i % 4 == 3) ? 255 : static_cast<uint8_t>(i * 17);
            DDS_FLAGS flags = DDS_FLAGS_FORCE_24BPP_RGB;
            if (kind == 3)
                flags |= DDS_FLAGS_FORCE_DX10_EXT;
            if (kind == 4)
                flags |= DDS_FLAGS_FORCE_DX10_EXT_MISC2;
            const bool legacy = kind < 3;
            Blob blob;
            CHECK(
                SUCCEEDED(SaveToDDSMemory(input.GetImages(), input.GetImageCount(), input.GetMetadata(), flags, blob)));
            size_t expectedSize = legacy ? 128 : 148;
            for (size_t i = 0; i < input.GetImageCount(); ++i)
                expectedSize += input.GetImages()[i].width * input.GetImages()[i].height * (legacy ? 3 : 4);
            CHECK(blob.GetBufferSize() == expectedSize);
            uint32_t pitch = 0, bitCount = 0;
            std::memcpy(&pitch, blob.GetBufferPointer() + 20, 4);
            std::memcpy(&bitCount, blob.GetBufferPointer() + 88, 4);
            CHECK(pitch == input.GetMetadata().width * (legacy ? 3 : 4));
            CHECK(bitCount == (legacy ? 24 : 0));
            CHECK(SUCCEEDED(
                LoadFromDDSMemory(blob.GetBufferPointer(), blob.GetBufferSize(), DDS_FLAGS_NONE, nullptr, loaded)));
            CHECK(loaded.GetImageCount() == input.GetImageCount());
            for (size_t i = 0; i < input.GetImageCount(); ++i)
            {
                const auto& src = input.GetImages()[i];
                const auto& dst = loaded.GetImages()[i];
                CHECK(dst.width == src.width && dst.height == src.height);
                for (size_t y = 0; y < src.height; ++y)
                {
                    for (size_t x = 0; x < src.width; ++x)
                    {
                        const auto* a = src.pixels + y * src.rowPitch + x * 4;
                        const auto* b = dst.pixels + y * dst.rowPitch + x * 4;
                        CHECK(b[0] == a[legacy ? 2 : 0] && b[1] == a[1] && b[2] == a[legacy ? 0 : 2] && b[3] == 255);
                    }
                }
            }
            CHECK(SUCCEEDED(
                SaveToDDSFile(input.GetImages(), input.GetImageCount(), input.GetMetadata(), flags, path.c_str())));
            CHECK(std::filesystem::file_size(path) == blob.GetBufferSize());
            CHECK(SUCCEEDED(LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, nullptr, fromFile)));
            Equal(loaded, fromFile);
            std::filesystem::remove(path);
            if (kind < 5)
                Record(loaded);
        }
    }

    void Processing()
    {
        auto input = ColorImage(8, 8, .25f, .5f, .75f, .5f);
        ScratchImage rgba, bgra, resized, mips, normals, premultiplied, copied, transformed;
        CHECK(SUCCEEDED(
            Convert(*input.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, TEX_FILTER_FORCE_NON_WIC, .5f, rgba)));
        CHECK(SUCCEEDED(
            Convert(*rgba.GetImage(0, 0, 0), DXGI_FORMAT_B8G8R8A8_UNORM, TEX_FILTER_FORCE_NON_WIC, .5f, bgra)));
        CHECK(bgra.GetPixels()[0] == rgba.GetPixels()[2]);
        CHECK(SUCCEEDED(Resize(*rgba.GetImage(0, 0, 0), 3, 5, TEX_FILTER_LINEAR | TEX_FILTER_FORCE_NON_WIC, resized)));
        CHECK(resized.GetImage(0, 0, 0)->height == 5);
        CHECK(SUCCEEDED(GenerateMipMaps(*rgba.GetImage(0, 0, 0), TEX_FILTER_BOX | TEX_FILTER_FORCE_NON_WIC, 0, mips)));
        CHECK(mips.GetImageCount() == 4 && mips.GetImage(3, 0, 0)->width == 1);
        CHECK(SUCCEEDED(
            ComputeNormalMap(*rgba.GetImage(0, 0, 0), CNMAP_CHANNEL_RED, 1.f, DXGI_FORMAT_R8G8B8A8_UNORM, normals)));
        CHECK(normals.GetPixels()[2] == 255);
        CHECK(SUCCEEDED(PremultiplyAlpha(*input.GetImage(0, 0, 0), TEX_PMALPHA_DEFAULT, premultiplied)));
        CHECK(reinterpret_cast<float*>(premultiplied.GetPixels())[0] == .125f);
        CHECK(SUCCEEDED(copied.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 2, 2, 1, 1)));
        CHECK(SUCCEEDED(CopyRectangle(*rgba.GetImage(0, 0, 0), Rect(1, 2, 2, 2), *copied.GetImage(0, 0, 0),
                                      TEX_FILTER_DEFAULT, 0, 0)));
        CHECK(std::memcmp(copied.GetPixels(), rgba.GetPixels(), 4) == 0);
        float mse = 1;
        CHECK(SUCCEEDED(ComputeMSE(*rgba.GetImage(0, 0, 0), *rgba.GetImage(0, 0, 0), mse, nullptr)) && mse == 0);
        size_t visited = 0;
        CHECK(SUCCEEDED(
            EvaluateImage(*rgba.GetImage(0, 0, 0), [&](const XMVECTOR*, size_t width, size_t) { visited += width; })));
        CHECK(visited == 64);
        CHECK(SUCCEEDED(TransformImage(
            *input.GetImage(0, 0, 0), [](XMVECTOR* out, const XMVECTOR* in, size_t width, size_t)
            { std::copy_n(in, width, out); }, transformed)));
        Equal(input, transformed);
        ScratchImage volume, volumeMips;
        CHECK(SUCCEEDED(volume.Initialize3D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4, 4, 1)));
        std::memset(volume.GetPixels(), 127, volume.GetPixelsSize());
        CHECK(SUCCEEDED(GenerateMipMaps3D(volume.GetImages(), volume.GetImageCount(), volume.GetMetadata(),
                                          TEX_FILTER_BOX, 0, volumeMips)));
        CHECK(volumeMips.GetMetadata().mipLevels == 3 && volumeMips.GetImageCount() == 7);
        for (auto p : {&rgba, &bgra, &resized, &mips, &normals, &premultiplied, &copied, &transformed, &volumeMips})
            Record(*p);
    }
    void OtherFormatsAndFiles()
    {
        auto input = ColorImage(8, 8, 4.f, 2.f, 1.f);
        ScratchImage decoded, rgba, dds;
        Blob blob;
        CHECK(SUCCEEDED(SaveToHDRMemory(*input.GetImage(0, 0, 0), blob)));
        CHECK(SUCCEEDED(LoadFromHDRMemory(blob.GetBufferPointer(), blob.GetBufferSize(), nullptr, decoded)));
        CHECK(std::abs(reinterpret_cast<float*>(decoded.GetPixels())[0] - 4.f) < .04f);
        Record(decoded);
        CHECK(SUCCEEDED(
            Convert(*input.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, TEX_FILTER_FORCE_NON_WIC, .5f, rgba)));
        CHECK(SUCCEEDED(SaveToTGAMemory(*rgba.GetImage(0, 0, 0), TGA_FLAGS_NONE, blob)));
        CHECK(SUCCEEDED(
            LoadFromTGAMemory(blob.GetBufferPointer(), blob.GetBufferSize(), TGA_FLAGS_NONE, nullptr, decoded)));
        Equal(rgba, decoded);
        Record(decoded);
        CHECK(FAILED(LoadFromHDRMemory(blob.GetBufferPointer(), 8, nullptr, decoded)));
        CHECK(FAILED(LoadFromTGAMemory(blob.GetBufferPointer(), 8, TGA_FLAGS_NONE, nullptr, decoded)));
        const auto directory = std::filesystem::current_path();
        const auto ddsPath   = (directory / "libdds-test.dds").wstring();
        const auto hdrPath   = (directory / "libdds-test.hdr").wstring();
        const auto tgaPath   = (directory / "libdds-test.tga").wstring();
        CHECK(SUCCEEDED(SaveToDDSFile(rgba.GetImages(), rgba.GetImageCount(), rgba.GetMetadata(), DDS_FLAGS_NONE,
                                      ddsPath.c_str())));
        CHECK(SUCCEEDED(LoadFromDDSFile(ddsPath.c_str(), DDS_FLAGS_NONE, nullptr, dds)));
        Equal(rgba, dds);
        CHECK(SUCCEEDED(SaveToHDRFile(*input.GetImage(0, 0, 0), hdrPath.c_str())));
        CHECK(SUCCEEDED(LoadFromHDRFile(hdrPath.c_str(), nullptr, decoded)));
        CHECK(SUCCEEDED(SaveToTGAFile(*rgba.GetImage(0, 0, 0), TGA_FLAGS_NONE, tgaPath.c_str())));
        CHECK(SUCCEEDED(LoadFromTGAFile(tgaPath.c_str(), TGA_FLAGS_NONE, nullptr, decoded)));
        Equal(rgba, decoded);
        for (const auto& p : {ddsPath, hdrPath, tgaPath})
            std::filesystem::remove(p);
    }
#ifndef LIBDDS_REFERENCE

    void DDS24BitBounds()
    {
        ScratchImage input;
        CHECK(SUCCEEDED(input.Initialize2D(DXGI_FORMAT_B8G8R8X8_UNORM, 2, 2, 1, 1)));
        const auto path = (std::filesystem::current_path() / "libdds-24bpp-invalid.dds").wstring();
        std::array<uint8_t, 24> wider{};
        for (int kind = 0; kind < 4; ++kind)
        {
            auto invalid = *input.GetImages();
            if (kind == 0)
                invalid.rowPitch = 4;
            if (kind == 1)
                invalid.slicePitch = 15;
            if (kind == 2)
                invalid = {3, 2, DXGI_FORMAT_B8G8R8X8_UNORM, 12, wider.size(), wider.data()};
            if (kind == 3)
            {
                invalid.rowPitch   = 6;
                invalid.slicePitch = 12;
            }
            Blob blob;
            CHECK(FAILED(SaveToDDSMemory(&invalid, 1, input.GetMetadata(), DDS_FLAGS_FORCE_24BPP_RGB, blob)));
            CHECK(FAILED(SaveToDDSFile(&invalid, 1, input.GetMetadata(), DDS_FLAGS_FORCE_24BPP_RGB, path.c_str())));
            std::filesystem::remove(path);
        }
        // Row padding is skipped, and no padding after the last row is required.
        std::array<uint8_t, 20> padded{1, 2, 3, 255, 4, 5, 6, 255, 99, 99, 99, 99, 7, 8, 9, 255, 10, 11, 12, 255};
        const Image source{2, 2, DXGI_FORMAT_B8G8R8X8_UNORM, 12, padded.size(), padded.data()};
        Blob blob;
        CHECK(SUCCEEDED(SaveToDDSMemory(&source, 1, input.GetMetadata(), DDS_FLAGS_FORCE_24BPP_RGB, blob)));
        const std::array<uint8_t, 12> expected{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
        CHECK(blob.GetBufferSize() == 128 + expected.size());
        CHECK(std::memcmp(blob.GetBufferPointer() + 128, expected.data(), expected.size()) == 0);
        CHECK(SUCCEEDED(SaveToDDSFile(&source, 1, input.GetMetadata(), DDS_FLAGS_FORCE_24BPP_RGB, path.c_str())));
        ScratchImage loaded, fromFile;
        CHECK(SUCCEEDED(
            LoadFromDDSMemory(blob.GetBufferPointer(), blob.GetBufferSize(), DDS_FLAGS_NONE, nullptr, loaded)));
        CHECK(SUCCEEDED(LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, nullptr, fromFile)));
        Equal(loaded, fromFile);
        std::filesystem::remove(path);
    }

#endif
}  // namespace
int main(int argc, char** argv)
{
    try
    {
        if (argc == 3 && std::string(argv[1]) == "--snapshot")
        {
            snapshot.open(argv[2], std::ios::binary);
            CHECK(snapshot.good());
        }
        KnownBlocks();
        BCFormats();
        Containers();
        UpstreamValidation();
        PitchBounds();
        HDRBounds();
        DDS24Bit();
        Processing();
        OtherFormatsAndFiles();
#ifndef LIBDDS_REFERENCE
        DDS24BitBounds();
#endif
        if (snapshot.is_open())
        {
            snapshot.flush();
            CHECK(snapshot.good());
        }
        std::cout << "All libdds tests passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Test failure: " << e.what() << '\n';
        return 1;
    }
}
