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
#include <AzCore/PlatformDef.h>

#if defined(AZ_PLATFORM_ANDROID)

#include <errno.h> // for EACCES

#include <AzCore/Android/APKFileHandler.h>
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Android/Utils.h>


//Note: Switching on verbose logging will give you a lot of detailed information about what files are being read from the APK
//      but there is a likelihood it could cause logcat to terminate with a 'buffer full' error. Restarting logcat will resume logging
//      but you may lose information
#define VERBOSE_IO_LOGGING 0

#if VERBOSE_IO_LOGGING
    #define FILE_IO_LOG(...) AZ_Printf("N3H5", __VA_ARGS__)
#else
    #define FILE_IO_LOG(...)
#endif

namespace AZ
{
    namespace Android
    {
        AZ::EnvironmentVariable<APKFileHandler> APKFileHandler::s_instance;


        bool APKFileHandler::Create()
        {
            if (!s_instance)
            {
                s_instance = AZ::Environment::CreateVariable<APKFileHandler>(AZ::AzTypeInfo<APKFileHandler>::Name());
            }

            if (s_instance->IsReady()) // already created in a different module
            {
                return true;
            }

            return s_instance->Initialize();
        }

        void APKFileHandler::Destroy()
        {
            s_instance.Reset();
        }


        FILE* APKFileHandler::Open(const char* filename, const char* mode, AZ::u64& size)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK Open");
            FILE* fileHandle = nullptr;

            if (mode[0] != 'w')
            {
                FILE_IO_LOG("******* Attempting to open file in APK:[%s] ", filename);

                AAsset* asset = AAssetManager_open(Utils::GetAssetManager(), Utils::StripApkPrefix(filename), AASSET_MODE_UNKNOWN);
                if (asset != nullptr)
                {
                    // the pointer returned by funopen will allow us to use fread, fseek etc
                    fileHandle = funopen(asset, APKFileHandler::Read, APKFileHandler::Write, APKFileHandler::Seek, APKFileHandler::Close);

                    // the file pointer we return from funopen can't be used to get the length of the file so we need to capture that info while we have the AAsset pointer available
                    size = static_cast<AZ::u64>(AAsset_getLength64(asset));
                    FILE_IO_LOG("File loaded successfully");
                }
                else
                {
                    FILE_IO_LOG("####### Failed to open file in APK:[%s] ", filename);
                }
            }
            return fileHandle;
        }

        int APKFileHandler::Read(void* asset, char* buffer, int size)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK Read");

            APKFileHandler& apkHandler = Get();

            if (apkHandler.m_numBytesToRead < size && apkHandler.m_numBytesToRead > 0)
            {
                size = apkHandler.m_numBytesToRead;
            }
            apkHandler.m_numBytesToRead -= size;

            return AAsset_read(static_cast<AAsset*>(asset), buffer, static_cast<size_t>(size));
        }

        int APKFileHandler::Write(void* asset, const char* buffer, int size)
        {
            return EACCES;
        }

        fpos_t APKFileHandler::Seek(void* asset, fpos_t offset, int origin)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK Seek");
            return AAsset_seek(static_cast<AAsset*>(asset), offset, origin);
        }

        int APKFileHandler::Close(void* asset)
        {
            AAsset_close(static_cast<AAsset*>(asset));
            return 0;
        }

        int APKFileHandler::FileLength(const char* filename)
        {
            AZ::u64 size = 0;
            FILE* asset = Open(filename, "r", size);
            if (asset != nullptr)
            {
                fclose(asset);
            }
            return static_cast<int>(size);
        }

        AZ::IO::Result APKFileHandler::ParseDirectory(const char* path, FindDirsCallbackType findCallback)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK ParseDirectory");
            FILE_IO_LOG("********* About to search for file in [%s] ******* ", path);

            APKFileHandler& apkHandler = Get();

            DirectoryCache::const_iterator it = apkHandler.m_cachedDirectories.find(path);
            if (it == apkHandler.m_cachedDirectories.end())
            {
                // The NDK version of the Asset Manager only returns files and not directories so we must use the Java version to get all the data we need
                JNIEnv* jniEnv = JNI::GetEnv();
                if (!jniEnv)
                {
                    return AZ::IO::ResultCode::Error;
                }

                auto newDirectory = apkHandler.m_cachedDirectories.emplace(path, StringVector());
                jstring dirPath = jniEnv->NewStringUTF(path);
                jobjectArray javaFileListObject = apkHandler.m_javaInstance->InvokeStaticObjectMethod<jobjectArray>("GetFilesAndDirectoriesInPath", dirPath);
                jniEnv->DeleteLocalRef(dirPath);

                int numObjects = jniEnv->GetArrayLength(javaFileListObject);
                bool parseResults = true;

                for (int i = 0; i < numObjects; i++)
                {
                    if (!parseResults)
                    {
                        break;
                    }

                    jstring str = static_cast<jstring>(jniEnv->GetObjectArrayElement(javaFileListObject, i));
                    const char* entryName = jniEnv->GetStringUTFChars(str, 0);
                    newDirectory.first->second.push_back(StringType(entryName));

                    parseResults = findCallback(entryName);

                    jniEnv->ReleaseStringUTFChars(str, entryName);
                    jniEnv->DeleteLocalRef(str);
                }

                jniEnv->DeleteGlobalRef(javaFileListObject);
            }
            else
            {
                bool parseResults = true;

                for (int i = 0; i < it->second.size(); i++)
                {
                    if (!parseResults)
                    {
                        break;
                    }

                    parseResults = findCallback(it->second[i].c_str());
                }
            }

            return AZ::IO::ResultCode::Success;
        }

        bool APKFileHandler::IsDirectory(const char* path)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK IsDir");

            APKFileHandler& apkHandler = Get();

            DirectoryCache::const_iterator it = apkHandler.m_cachedDirectories.find(path);
            if (it == apkHandler.m_cachedDirectories.end())
            {
                JNIEnv* jniEnv = JNI::GetEnv();
                if (!jniEnv)
                {
                    return false;
                }

                jstring dirPath = jniEnv->NewStringUTF(path);
                jboolean isDir = apkHandler.m_javaInstance->InvokeStaticBooleanMethod("IsDirectory", dirPath);
                jniEnv->DeleteLocalRef(dirPath);

                FILE_IO_LOG("########### [%s] %s a directory ######### ", path, isDir == JNI_TRUE ? "IS" : "IS NOT");

                return (isDir == JNI_TRUE);
            }
            else
            {
                return (it->second.size() > 0);
            }
        }

        bool APKFileHandler::DirectoryOrFileExists(const char* path)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK DirOrFileexists");

            AZ::OSString file(Utils::StripApkPrefix(path));
            AZ::OSString temp(file);

            auto pos = file.find_last_of('/');
            auto filename = file.substr(pos + 1);
            auto pathToFile = temp.substr(0, pos);
            bool foundFile = false;

            ParseDirectory(pathToFile.c_str(), [&](const char* name)
            {
                if (strcasecmp(name, filename.c_str()) == 0)
                {
                    foundFile = true;
                }

                return true;
            });

            FILE_IO_LOG("########### Directory or file [%s] %s exist ######### ", filename.c_str(), foundFile ? "DOES" : "DOES NOT");
            return foundFile;
        }

        void APKFileHandler::SetNumBytesToRead(const size_t numBytesToRead)
        {
            // WARNING: This isn't a thread safe way of handling this problem, LY-65478 will fix it
            APKFileHandler& apkHandler = Get();
            apkHandler.m_numBytesToRead = numBytesToRead;
        }


        APKFileHandler::APKFileHandler()
                : m_javaInstance()
                , m_cachedDirectories()
                , m_numBytesToRead(0)
        {
        }

        APKFileHandler::~APKFileHandler()
        {
            if (s_instance)
            {
                AZ_Assert(s_instance.IsOwner(), "The Android APK file handler instance is being destroyed by someone other than the owner.");
            }
        }


        APKFileHandler& APKFileHandler::Get()
        {
            if (!s_instance)
            {
                s_instance = AZ::Environment::FindVariable<APKFileHandler>(AZ::AzTypeInfo<APKFileHandler>::Name());
                AZ_Assert(s_instance, "The Android APK file handler is NOT ready for use! Call Create first!");
            }
            return *s_instance;
        }


        bool APKFileHandler::Initialize()
        {
            JniObject* apkHandler = aznew JniObject("com/uuzu/n3h5app/io/APKHandler", "APKHandler");
            if (!apkHandler)
            {
                return false;
            }

            m_javaInstance.reset(apkHandler);

            m_javaInstance->RegisterStaticMethod("IsDirectory", "(Ljava/lang/String;)Z");
            m_javaInstance->RegisterStaticMethod("GetFilesAndDirectoriesInPath", "(Ljava/lang/String;)[Ljava/lang/String;");

        #if VERBOSE_IO_LOGGING
            m_javaInstance->RegisterStaticField("s_debug", "Z");
            m_javaInstance->SetStaticBooleanField("s_debug", JNI_TRUE);
        #endif

            return true;
        }

        bool APKFileHandler::IsReady() const
        {
            return (m_javaInstance != nullptr);
        }

    } // namespace Android
} // namespace AZ

#endif // defined(AZ_PLATFORM_ANDROID)

