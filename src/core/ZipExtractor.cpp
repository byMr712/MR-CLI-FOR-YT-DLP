#include "ZipExtractor.hpp"
#include "WinLibs.hpp"
#include "Handle.hpp"
#include "FileSystem.hpp"

namespace mrcli::core {

bool ZipExtractor::extract(
    const PathArg& zipFilePath,
    const PathArg& destinationDirectory
) {
    if (!FileSystem::fileExists(zipFilePath)) {
        return false;
    }
    FileSystem::createDirectories(destinationDirectory);

    ScopedCoInit coInit;
    IShellDispatch* pShell = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_Shell,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IShellDispatch,
        reinterpret_cast<void**>(&pShell)
    );
    if (FAILED(hr) || !pShell) {
        return false;
    }

    std::wstring wzip = zipFilePath.path().wstring();
    std::wstring wdest = destinationDirectory.path().wstring();

    VARIANT vZip;
    VariantInit(&vZip);
    vZip.vt = VT_BSTR;
    vZip.bstrVal = SysAllocString(wzip.c_str());

    VARIANT vDest;
    VariantInit(&vDest);
    vDest.vt = VT_BSTR;
    vDest.bstrVal = SysAllocString(wdest.c_str());

    Folder* pZipFolder = nullptr;
    Folder* pDestFolder = nullptr;

    bool success = false;
    if (SUCCEEDED(pShell->NameSpace(vZip, &pZipFolder)) && pZipFolder &&
        SUCCEEDED(pShell->NameSpace(vDest, &pDestFolder)) && pDestFolder) {

        FolderItems* pItems = nullptr;
        if (SUCCEEDED(pZipFolder->Items(&pItems)) && pItems) {
            VARIANT vItems;
            VariantInit(&vItems);
            vItems.vt = VT_DISPATCH;
            vItems.pdispVal = pItems;

            // FOF_NO_UI = 4 | 16 | 512 | 1024 = 1556
            VARIANT vOptions;
            VariantInit(&vOptions);
            vOptions.vt = VT_I4;
            vOptions.lVal = 4 | 16 | 512 | 1024;

            hr = pDestFolder->CopyHere(vItems, vOptions);
            success = SUCCEEDED(hr);

            pItems->Release();
        }
    }

    if (pZipFolder) pZipFolder->Release();
    if (pDestFolder) pDestFolder->Release();
    VariantClear(&vZip);
    VariantClear(&vDest);
    pShell->Release();

    return success;
}

} // namespace mrcli::core
