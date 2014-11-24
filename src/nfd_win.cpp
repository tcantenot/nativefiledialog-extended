/*
  Native File Dialog

  http://www.frogtoss.com/labs
 */

#include <wchar.h>
#include <assert.h>
#include <windows.h>
#include <ShObjIdl.h>

#include "nfd_common.h"

#define UNICODE

// allocs the space in outPath -- call free()
// todo: outPath to outStr
static void CopyWCharToNFDChar( const wchar_t *inStr, nfdchar_t **outPath )
{
    int inStrCharacterCount = static_cast<int>(wcslen(inStr)); 
    int bytesNeeded = WideCharToMultiByte( CP_UTF8, 0,
                                           inStr, inStrCharacterCount,
                                           NULL, 0, NULL, NULL );    
    assert( bytesNeeded );
    assert( !*outPath );
    bytesNeeded += 1;

    *outPath = (nfdchar_t*)NFDi_Malloc( bytesNeeded );
    if ( !*outPath )
        return;

    int ret = WideCharToMultiByte( CP_UTF8, 0,
                                   inStr, -1,
                                   *outPath, bytesNeeded,
                                   NULL, NULL );
    assert( ret ); _NFD_UNUSED(ret);
}

// allocs the space in outStr -- call free()
static void CopyNFDCharToWChar( const nfdchar_t *inStr, wchar_t **outStr )
{
    
    int inStrByteCount = static_cast<int>(strlen(inStr));
    int charsNeeded = MultiByteToWideChar(CP_UTF8, 0,
                                          inStr, inStrByteCount,
                                          NULL, 0 );    
    assert( charsNeeded );
    assert( !*outStr );
    charsNeeded += 1; // terminator
    
    *outStr = (wchar_t*)NFDi_Malloc( charsNeeded * sizeof(wchar_t) );
    if ( !*outStr )
        return;        

    int ret = MultiByteToWideChar(CP_UTF8, 0,
                                  inStr, inStrByteCount,
                                  *outStr, charsNeeded);
    (*outStr)[charsNeeded-1] = '\0';

#ifdef _DEBUG
    int inStrCharacterCount = static_cast<int>(NFDi_UTF8_Strlen(inStr));
    assert( ret == inStrCharacterCount );
#else
    _NFD_UNUSED(ret);
#endif
}

static void AddFiltersToDialog( ::IFileOpenDialog *fileOpenDialog, const char *filterList )
{
    if ( !filterList || strlen(filterList) == 0 )
        return;

    COMDLG_FILTERSPEC spec;
    spec.pszName = NULL;
    spec.pszSpec = NULL;
    CopyNFDCharToWChar( "graphics", (wchar_t**)&spec.pszName );
    CopyNFDCharToWChar( "*.png", (wchar_t**)&spec.pszSpec );

    fileOpenDialog->SetFileTypes( 1, &spec );
    free( (void*)spec.pszSpec );
    free( (void*)spec.pszName );
}

/* public */

nfdresult_t NFD_OpenDialog( const char *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
    // Init COM library.
    HRESULT result = ::CoInitializeEx(NULL,
                                      ::COINIT_APARTMENTTHREADED |
                                      ::COINIT_DISABLE_OLE1DDE );
    if ( !SUCCEEDED(result))
    {
        NFDi_SetError("Could not initialize COM.");
        return NFD_ERROR;
    }

    ::IFileOpenDialog *fileOpenDialog(NULL);

    // Create dialog
    result = ::CoCreateInstance(::CLSID_FileOpenDialog, NULL,
                                CLSCTX_ALL, ::IID_IFileOpenDialog,
                                reinterpret_cast<void**>(&fileOpenDialog) );
                                
    if ( !SUCCEEDED(result) )
    {
        NFDi_SetError("Could not create dialog.");
        return NFD_ERROR;
    }

    // Build the filter list
    AddFiltersToDialog( fileOpenDialog, filterList );

    // Show the dialog.
    result = fileOpenDialog->Show(NULL);
    nfdresult_t nfdResult = NFD_ERROR;
    if ( SUCCEEDED(result) )
    {
        // Get the file name
        ::IShellItem *shellItem(NULL);
        result = fileOpenDialog->GetResult(&shellItem);
        if ( !SUCCEEDED(result) )
        {
            NFDi_SetError("Could not get shell item from dialog.");
            return NFD_ERROR;
        }
        wchar_t *filePath(NULL);
        result = shellItem->GetDisplayName(::SIGDN_FILESYSPATH, &filePath);
        if ( !SUCCEEDED(result) )
        {
            NFDi_SetError("Could not get file path for selected.");
            return NFD_ERROR;
        }

        CopyWCharToNFDChar( filePath, outPath );
        CoTaskMemFree(filePath);
        if ( !*outPath )
        {
            /* error is malloc-based, error message would be redundant */
            return NFD_ERROR;
        }

        nfdResult = NFD_OKAY;
        
        shellItem->Release();
    }
    else if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED) )
    {
        nfdResult = NFD_CANCEL;
    }
    else
    {
        NFDi_SetError("File dialog box show failed.");
        nfdResult = NFD_ERROR;
    }

    ::CoUninitialize();
    
    return nfdResult;
}

nfdresult_t NFD_OpenDialogMultiple( const nfdchar_t *filterList,
                                    const nfdchar_t *defaultPath,
                                    nfdpathset_t *outPaths )
{
    return NFD_ERROR;
}

nfdresult_t NFD_SaveDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath )
{
    
    return NFD_ERROR;
}