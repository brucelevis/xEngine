/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/RTTI/RTTI.h>

// This header file and CPP handles the platform specific implementation of code as defined by the FileIOBase interface class.
// In order to make your code portable and functional with both this and the RemoteFileIO class, use the interface to access
// the methods.

namespace AZ
{
    namespace IO
    {
#if defined(AZ_PLATFORM_ANDROID)
        typedef FILE* OSHandleType; // Reserve a type so that its easy to convert to CreateFile for example
        struct FileDescriptor
        {
            AZ::OSString m_filename;
            OSHandleType m_handle;
            AZ::u64      m_size;
            bool         m_isPackaged;
        };
#else
        using FileDescriptor = SystemFile;
#endif


        class LocalFileIO
            : public FileIOBase
        {
        public:
            AZ_RTTI(LocalFileIO, "{87A8D32B-F695-4105-9A4D-D99BE15DFD50}");
            AZ_CLASS_ALLOCATOR(LocalFileIO, OSAllocator, 0);

            LocalFileIO();
            ~LocalFileIO();

            Result Open(const char* filePath, OpenMode mode, HandleType& fileHandle) override;
            Result Close(HandleType fileHandle) override;
            Result Tell(HandleType fileHandle, AZ::u64& offset) override;
            Result Seek(HandleType fileHandle, AZ::s64 offset, SeekType type) override;
            Result Size(HandleType fileHandle, AZ::u64& size) override;
            Result Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead = false, AZ::u64* bytesRead = nullptr) override;
            Result Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten = nullptr) override;
            Result Flush(HandleType fileHandle) override;
            bool Eof(HandleType fileHandle) override;
            AZ::u64 ModificationTime(HandleType fileHandle) override;

            bool Exists(const char* filePath) override;
            Result Size(const char* filePath, AZ::u64& size) override;
            AZ::u64 ModificationTime(const char* filePath) override;

            bool IsDirectory(const char* filePath) override;
            bool IsReadOnly(const char* filePath) override;
            Result CreatePath(const char* filePath) override;
            Result DestroyPath(const char* filePath) override;
            Result Remove(const char* filePath) override;
            Result Copy(const char* sourceFilePath, const char* destinationFilePath) override;
            Result Rename(const char* originalFilePath, const char* newFilePath) override;
            Result FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback) override;

            void SetAlias(const char* alias, const char* path) override;
            void ClearAlias(const char* alias) override;
            const char* GetAlias(const char* alias) override;
            AZ::u64 ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const override;
            bool ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) override;

            bool GetFilename(HandleType fileHandle, char* filename, AZ::u64 filenameSize) const override;
            bool ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength) const;
            
        private:
            typedef AZStd::pair<AZ::OSString, AZ::OSString> AliasType;

#if defined(AZ_PLATFORM_ANDROID)
            OSHandleType GetFilePointerFromHandle(HandleType fileHandle);
#else
            SystemFile* GetFilePointerFromHandle(HandleType fileHandle);
#endif
            const FileDescriptor* GetFileDescriptorFromHandle(HandleType fileHandle);

            HandleType GetNextHandle();
            
            bool ResolveAliases(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const;
            bool IsAbsolutePath(const char* path) const;

        private:
            static AZ::OSString RemoveTrailingSlash(const AZ::OSString& pathStr);
            static AZ::OSString CheckForTrailingSlash(const AZ::OSString& pathStr);

            mutable AZStd::recursive_mutex m_openFileGuard;
            AZStd::atomic<HandleType> m_nextHandle;
            AZStd::map<HandleType, FileDescriptor, AZStd::less<HandleType>, AZ::OSStdAllocator> m_openFiles;
            AZStd::vector<AliasType, AZ::OSStdAllocator> m_aliases;

            void CheckInvalidWrite(const char* path);
        };
    } // namespace IO
} // namespace AZ

