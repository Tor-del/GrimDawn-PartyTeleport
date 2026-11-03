#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <bcrypt.h>
#include <string>
#include <vector>
#include <fstream>
#pragma comment(lib, "bcrypt.lib")

inline std::string sha256(const std::wstring& path) {
    BCRYPT_ALG_HANDLE alg{}; BCRYPT_HASH_HANDLE hash{};
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) return {};
    DWORD objectSize{}, count{};
    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize),
                          sizeof(objectSize), &count, 0) < 0) { BCryptCloseAlgorithmProvider(alg, 0); return {}; }
    std::vector<UCHAR> object(objectSize);
    if (BCryptCreateHash(alg, &hash, object.data(), objectSize, nullptr, 0, 0) < 0) {
        BCryptCloseAlgorithmProvider(alg, 0); return {};
    }
    std::ifstream file(path, std::ios::binary);
    char buffer[65536]; bool ok = file.is_open();
    while (ok && file) {
        file.read(buffer, sizeof(buffer));
        if (BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer), static_cast<ULONG>(file.gcount()), 0) < 0) ok = false;
    }
    ok = ok && !file.bad();
    UCHAR digest[32]{};
    ok = ok && BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
    BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(alg, 0);
    if (!ok) return {};
    const char* hex = "0123456789abcdef";
    std::string result;
    for (auto b : digest) { result += hex[b >> 4]; result += hex[b & 15]; }
    return result;
}
inline bool supported(const std::wstring& gameDir) {
    return sha256(gameDir + L"\\Game.dll") == "cd7ecd7942d719f96b42aff4896f8c25d52152b4a41191f71a4a66ea48cecad2" &&
           sha256(gameDir + L"\\Engine.dll") == "4c2ea14ce1c591b315cc29a8a96819a5c3c7e194adbd6f7eea208b9bb8a93153";
}
