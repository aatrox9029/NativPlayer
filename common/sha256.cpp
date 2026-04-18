#include "common/sha256.h"

#include <Windows.h>
#include <bcrypt.h>

#include <vector>

namespace velo::common {
namespace {

std::wstring FormatHex(const BYTE* data, const ULONG size) {
    static constexpr wchar_t kHex[] = L"0123456789abcdef";
    std::wstring output;
    output.reserve(static_cast<size_t>(size) * 2);
    for (ULONG index = 0; index < size; ++index) {
        output.push_back(kHex[(data[index] >> 4) & 0x0F]);
        output.push_back(kHex[data[index] & 0x0F]);
    }
    return output;
}

}  // namespace

bool TryComputeFileSha256Hex(const std::filesystem::path& path, std::wstring& sha256Hex, std::wstring& errorMessage) {
    sha256Hex.clear();
    errorMessage.clear();

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    HANDLE file = INVALID_HANDLE_VALUE;
    std::vector<UCHAR> objectBuffer;
    std::vector<UCHAR> hashBuffer;

    const auto cleanup = [&]() {
        if (hash != nullptr) {
            BCryptDestroyHash(hash);
        }
        if (algorithm != nullptr) {
            BCryptCloseAlgorithmProvider(algorithm, 0);
        }
        if (file != INVALID_HANDLE_VALUE) {
            CloseHandle(file);
        }
    };

    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
        errorMessage = L"Failed to initialize SHA-256 provider.";
        cleanup();
        return false;
    }

    DWORD objectLength = 0;
    ULONG size = sizeof(objectLength);
    if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &size, 0) != 0) {
        errorMessage = L"Failed to query SHA-256 object length.";
        cleanup();
        return false;
    }

    DWORD hashLength = 0;
    if (BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLength), sizeof(hashLength), &size, 0) != 0) {
        errorMessage = L"Failed to query SHA-256 digest length.";
        cleanup();
        return false;
    }

    objectBuffer.resize(objectLength);
    hashBuffer.resize(hashLength);
    if (BCryptCreateHash(algorithm, &hash, objectBuffer.data(), static_cast<ULONG>(objectBuffer.size()), nullptr, 0, 0) != 0) {
        errorMessage = L"Failed to create SHA-256 hash.";
        cleanup();
        return false;
    }

    file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        errorMessage = L"Failed to open file for hashing: " + path.wstring();
        cleanup();
        return false;
    }

    std::vector<BYTE> readBuffer(64 * 1024);
    while (true) {
        DWORD bytesRead = 0;
        if (!ReadFile(file, readBuffer.data(), static_cast<DWORD>(readBuffer.size()), &bytesRead, nullptr)) {
            errorMessage = L"Failed while hashing file: " + path.wstring();
            cleanup();
            return false;
        }
        if (bytesRead == 0) {
            break;
        }
        if (BCryptHashData(hash, readBuffer.data(), bytesRead, 0) != 0) {
            errorMessage = L"Failed to update SHA-256 hash.";
            cleanup();
            return false;
        }
    }

    if (BCryptFinishHash(hash, hashBuffer.data(), static_cast<ULONG>(hashBuffer.size()), 0) != 0) {
        errorMessage = L"Failed to finalize SHA-256 hash.";
        cleanup();
        return false;
    }

    sha256Hex = FormatHex(hashBuffer.data(), static_cast<ULONG>(hashBuffer.size()));
    cleanup();
    return true;
}

}  // namespace velo::common
