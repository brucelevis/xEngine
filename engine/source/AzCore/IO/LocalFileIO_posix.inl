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
#include <fstream>
#include <sys/time.h>
#include <sys/stat.h>
#include <utime.h>
#include <fcntl.h>
#include <dirent.h>
#include <AzCore/std/string/osstring.h>
#include <unistd.h>

namespace AZ
{
    namespace IO
    {
        bool LocalFileIO::IsDirectory(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN] = {0};
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            struct stat result;
            if (stat(resolvedPath, &result) == 0)
            {
                return S_ISDIR(result.st_mode);
            }
            return false;
        }

        Result LocalFileIO::Copy(const char* sourceFilePath, const char* destinationFilePath)
        {
            char resolvedSourceFilePath[AZ_MAX_PATH_LEN] = {0};
            ResolvePath(sourceFilePath, resolvedSourceFilePath, AZ_MAX_PATH_LEN);

            char resolvedDestinationFilePath[AZ_MAX_PATH_LEN] = {0};
            ResolvePath(destinationFilePath, resolvedDestinationFilePath, AZ_MAX_PATH_LEN);

            // Use standard C++ method of file copy.
            {
                std::ifstream  src(resolvedSourceFilePath, std::ios::binary);
                if (src.fail())
                {
                    return ResultCode::Error;
                }
                std::ofstream  dst(resolvedDestinationFilePath, std::ios::binary);

                if (dst.fail())
                {
                    return ResultCode::Error;
                }
                dst << src.rdbuf();
            }
            return ResultCode::Success;
        }

        Result LocalFileIO::FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback)
        {
            char resolvedPath[AZ_MAX_PATH_LEN] = {0};
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            AZ::OSString withoutSlash = RemoveTrailingSlash(resolvedPath);
            DIR* dir = opendir(withoutSlash.c_str());

            if (dir != NULL)
            {
                struct dirent entry;
                struct dirent* result = NULL;
                // because the absolute path might actually be SHORTER than the alias ("c:/r/dev" -> "@devroot@"), we need to
                // use a static buffer here.
                char tempBuffer[AZ_MAX_PATH_LEN];

                int lastError = readdir_r(dir, &entry, &result);
                // List all the other files in the directory.
                while (result != NULL && lastError == 0)
                {
                    if (NameMatchesFilter(entry.d_name, filter))
                    {
                        AZ::OSString foundFilePath = CheckForTrailingSlash(resolvedPath);
                        foundFilePath += entry.d_name;
                        // if aliased, dealias!
                        azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, foundFilePath.c_str());
                        ConvertToAlias(tempBuffer, AZ_MAX_PATH_LEN);

                        if (!callback(tempBuffer))
                        {
                            break;
                        }
                    }
                    lastError = readdir_r(dir, &entry, &result);
                }

                if (lastError != 0)
                {
                    closedir(dir);
                    return ResultCode::Error;
                }

                closedir(dir);
                return ResultCode::Success;
            }
            else
            {
                return ResultCode::Error;
            }
        }

        Result LocalFileIO::CreatePath(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN] = {0};
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            // create all paths up to that directory.
            // its not an error if the path exists.
            if ((Exists(resolvedPath)) && (!IsDirectory(resolvedPath)))
            {
                return ResultCode::Error; // that path exists, but is not a directory.
            }

            // make directories from bottom to top.
            AZ::OSString buf;
            size_t pathLength = strlen(resolvedPath);
            buf.reserve(pathLength);
            for (size_t pos = 0; pos < pathLength; ++pos)
            {
                if ((resolvedPath[pos] == '\\') || (resolvedPath[pos] == '/'))
                {
                    if (pos > 0)
                    {
                        mkdir(buf.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                        if (!IsDirectory(buf.c_str()))
                        {
                            return ResultCode::Error;
                        }
                    }
                }
                buf.push_back(resolvedPath[pos]);
            }

            mkdir(buf.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            return IsDirectory(resolvedPath) ? ResultCode::Success : ResultCode::Error;
        }

        bool LocalFileIO::IsAbsolutePath(const char* path) const
        {
            return path && path[0] == '/';
        }

        bool LocalFileIO::ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength) const
        {
            AZ_Assert(maxLength >= AZ_MAX_PATH_LEN, "Path length is larger than AZ_MAX_PATH_LEN");
            if (!IsAbsolutePath(path))
            {
                // note that realpath fails if the path does not exist and actually changes the return value
                // to be the actual place that FAILED, which we don't want.
                // if we fail, we'd prefer to fall through and at least use the original path.
                const char* result = realpath(path, absolutePath);
                if (result)
                {
                    size_t len = ::strlen(absolutePath);
                    if (len > 0)
                    {
                        // strip trailing slash
                        if (absolutePath[len - 1] == '/')
                        {
                            absolutePath[len - 1] = 0;
                        }
                    }
                    return true;
                }
            }
            azstrcpy(absolutePath, maxLength, path);
            size_t len = ::strlen(absolutePath);
            if (len > 0)
            {
                // strip trailing slash
                if (absolutePath[len - 1] == '/')
                {
                    absolutePath[len - 1] = 0;
                }
            }
            return IsAbsolutePath(absolutePath);
        }
    } // namespace IO
} // namespace AZ
