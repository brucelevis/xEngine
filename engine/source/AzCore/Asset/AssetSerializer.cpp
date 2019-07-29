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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/SystemFile.h>

namespace AZ {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    const Uuid& GetAssetClassId()
    {
        static Uuid s_typeId("{77A19D40-8731-4d3c-9041-1B43047366A4}");
        return s_typeId;
    }
    //-------------------------------------------------------------------------
    AssetSerializer AssetSerializer::s_serializer;
    //-------------------------------------------------------------------------

    size_t AssetSerializer::DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/)
    {
        (void)isDataBigEndian;
        const size_t dataSize = sizeof(Data::AssetId) + sizeof(Data::AssetType);
        AZ_Assert(in.GetLength() >= dataSize, "Invalid data in stream");
        (void)dataSize;

        Data::AssetId assetId;
        Data::AssetType assetType;
        size_t hintSize = 0;
        AZStd::string assetHint;
        in.Read(sizeof(Data::AssetId), reinterpret_cast<void*>(&assetId));
        in.Read(sizeof(assetType), reinterpret_cast<void*>(&assetType));
        in.Read(sizeof(size_t), reinterpret_cast<void*>(&hintSize));
        AZ_SERIALIZE_SWAP_ENDIAN(assetId.m_subId, isDataBigEndian);
        AZ_SERIALIZE_SWAP_ENDIAN(hintSize, isDataBigEndian);
        assetHint.resize(hintSize);
        in.Read(hintSize, reinterpret_cast<void*>(assetHint.data()));

        AZStd::string outText = AZStd::string::format("id=%s,type=%s,hint={%s}", assetId.ToString<AZStd::string>().c_str(), assetType.ToString<AZStd::string>().c_str(), assetHint.c_str());
        return static_cast<size_t>(out.Write(outText.size(), outText.c_str()));
    }

    //-------------------------------------------------------------------------

    bool AssetSerializer::Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian)
    {
        using namespace AZ::Data;

        (void)isDataBigEndian;

        // version 0 just has asset Id and type
        size_t dataSize = sizeof(AssetId) + sizeof(AssetType);
        if (version > 0)
        {
            dataSize += sizeof(IO::SizeType); // There must be at least enough room for the hint length
        }
        if (stream.GetLength() < dataSize)
        {
            return false;
        }

        AssetId assetId = AssetId();
        AssetType assetType = AssetType::CreateNull();
        AZStd::string assetHint;

        stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

        AZ::IO::SizeType bytesRead = 0;
        bytesRead += stream.Read(sizeof(assetId), reinterpret_cast<void*>(&assetId));
        AZ_SERIALIZE_SWAP_ENDIAN(assetId.m_subId, isDataBigEndian);
        bytesRead += stream.Read(sizeof(assetType), reinterpret_cast<void*>(&assetType));
        if (version > 0)
        {
            IO::SizeType hintSize = 0;
            bytesRead += stream.Read(sizeof(hintSize), reinterpret_cast<void*>(&hintSize));
            AZ_SERIALIZE_SWAP_ENDIAN(hintSize, isDataBigEndian);
            AZ_Warning("Asset", hintSize < AZ_MAX_PATH_LEN, "Invalid asset hint, will be truncated");
            hintSize = AZStd::min<size_t>(hintSize, AZ_MAX_PATH_LEN);
            assetHint.resize(hintSize);
            dataSize += hintSize;
            bytesRead += stream.Read(hintSize, reinterpret_cast<void*>(assetHint.data()));
        }
        
        AZ_Assert(bytesRead == dataSize, "Invalid asset type/read");
        (void)bytesRead;


        Asset<AssetData>* asset = reinterpret_cast<Asset<AssetData>*>(classPtr);
        
        asset->m_assetId = assetId;
        asset->m_assetType = assetType;
        asset->m_assetHint = assetHint;
   
        return true;
    }

    //-------------------------------------------------------------------------

    size_t  AssetSerializer::TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian)
    {
        (void)isDataBigEndian;
        // Parse the asset id and type
        const char* idGuidStart = strchr(text, '{');
        AZ_Assert(idGuidStart, "Invalid asset guid data! %s", text);
        const char* idGuidEnd = strchr(idGuidStart, ':');
        AZ_Assert(idGuidEnd, "Invalid asset guid data! %s", idGuidStart);
        const char* idSubIdStart = idGuidEnd + 1;
        const char* idSubIdEnd = strchr(idSubIdStart, ',');
        AZ_Assert(idSubIdEnd, "Invalid asset subId data! %s", idSubIdStart);
        const char* idTypeStart = strchr(idSubIdEnd, '{');
        AZ_Assert(idTypeStart, "Invalid asset type data! %s", idSubIdEnd);
        const char* idTypeEnd = strchr(idTypeStart, '}');
        AZ_Assert(idTypeEnd, "Invalid asset type data! %s", idTypeStart);
        idTypeEnd++;

        // Read hint for version >= 1
        AZStd::string assetHint;
        if (textVersion > 0)
        {
            const char* hintStart = strchr(idTypeEnd, '{');
            AZ_Assert(hintStart, "Invalid asset hint data! %s", idTypeEnd);
            const char* hintEnd = strchr(hintStart, '}');
            AZ_Assert(hintEnd, "Invalid asset hint data! %s", hintStart);
            assetHint.assign(hintStart+1, hintEnd);
        }
        
        Data::AssetId assetId;
        assetId.m_guid = Uuid::CreateString(idGuidStart, idGuidEnd - idGuidStart);
        assetId.m_subId = static_cast<u32>(strtoul(idSubIdStart, nullptr, 16));
        Data::AssetType assetType = Uuid::CreateString(idTypeStart, idTypeEnd - idTypeStart);

        Data::Asset<Data::AssetData> asset(assetId, assetType, assetHint);
        return Save(&asset, stream, isDataBigEndian);
    }

    size_t AssetSerializer::Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian)
    {
        (void)isDataBigEndian;

        const Data::Asset<Data::AssetData>* asset = reinterpret_cast<const Data::Asset<Data::AssetData>*>(classPtr);

        AZ_Assert(asset->Get() == nullptr || asset->GetType() != AzTypeInfo<Data::AssetData>::Uuid(), 
            "Asset contains data, but does not have a valid asset type.");

        Data::AssetId assetId = asset->GetId();
        Data::AssetType assetType = asset->GetType();
        const AZStd::string& assetHint = asset->GetHint();
        IO::SizeType assetHintSize = assetHint.size();

        AZ_SERIALIZE_SWAP_ENDIAN(assetId.m_subId, isDataBigEndian);
        AZ_SERIALIZE_SWAP_ENDIAN(assetHintSize, isDataBigEndian);

        stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        size_t bytesWritten = static_cast<size_t>(stream.Write(sizeof(Data::AssetId), reinterpret_cast<void*>(&assetId)));
        bytesWritten += static_cast<size_t>(stream.Write(sizeof(Data::AssetType), reinterpret_cast<void*>(&assetType)));
        bytesWritten += static_cast<size_t>(stream.Write(sizeof(assetHintSize), reinterpret_cast<void*>(&assetHintSize)));
        bytesWritten += static_cast<size_t>(stream.Write(assetHint.size(), assetHint.c_str()));
        return bytesWritten;
    }

    bool AssetSerializer::CompareValueData(const void* lhs, const void* rhs)
    {
        return SerializeContext::EqualityCompareHelper<Data::Asset<Data::AssetData>>::CompareValues(lhs, rhs);
    }


    //-------------------------------------------------------------------------
}   // namespace AZ
#endif // #ifndef AZ_UNITY_BUILD