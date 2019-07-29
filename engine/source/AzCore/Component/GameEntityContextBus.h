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

/** 
 * @file
 * Header file for buses that dispatch and receive events 
 * from the game entity context. 
 * The game entity context holds gameplay entities, as opposed 
 * to system entities, editor entities, and so on.
 */

#ifndef AZCORE_GAMEENTITYCONTEXTBUS_H
#define AZCORE_GAMEENTITYCONTEXTBUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Component/BehaviorEntity.h>
#include <AzCore/Serialization/IdUtils.h>

namespace AZ
{
    class Entity;
}

namespace AZ
{
    /**
     * Interface for AzFramework::GameEntityContextRequestBus, which is  
     * the EBus that makes requests to the game entity context. 
     * The game entity context holds gameplay entities, as opposed
     * to system entities, editor entities, and so on.
     */
    class GameEntityContextRequests
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~GameEntityContextRequests() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        /**
         * Overrides the default AZ::EBusTraits handler policy so that this 
         * EBus supports a single handler at each address. This EBus has only  
         * one handler because it uses the default AZ::EBusTraits address 
         * policy, and that policy specifies that the EBus has only one address.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /**
         * Gets the ID of the game entity context.
         * @return The ID of the game entity context.
         */
        virtual EntityContextId GetGameEntityContextId() = 0;

        /**
         * Creates an entity in the game context.
         * @param name A name for the new entity.
         * @return A pointer to a new entity.
         */
        virtual AZ::Entity* CreateGameEntity(const char* /*name*/) = 0;

        /**
         * Creates an entity in the game context.
         * @param name A name for the new entity.
         * @return An entity wrapper for use within the BehaviorContext.
         */
        virtual BehaviorEntity CreateGameEntityForBehaviorContext(const char* /*name*/) = 0;

        /**
         * Adds an existing entity to the game context.
         * @param entity A pointer to the entity to add to the game context.
         */
        virtual void AddGameEntity(AZ::Entity* /*entity*/) = 0;

        /**
         * Destroys an entity. 
         * The entity is immediately deactivated and will be destroyed on the next tick.
         * @param id The ID of the entity to destroy.
         */
        virtual void DestroyGameEntity(const AZ::EntityId& /*id*/) = 0;

        /**
         * Destroys an entity and all of its descendants. 
         * The entity and its descendants are immediately deactivated and will be 
         * destroyed on the next tick.
         * @param id The ID of the entity to destroy.
         */
        virtual void DestroyGameEntityAndDescendants(const AZ::EntityId& /*id*/) = 0;

        /**
         * Activates the game entity.
         * @param id The ID of the entity to activate.
         */
        virtual void ActivateGameEntity(const AZ::EntityId& /*id*/) = 0;
        
        /**
         * Deactivates the game entity.
         * @param id The ID of the entity to deactivate.
         */
        virtual void DeactivateGameEntity(const AZ::EntityId& /*id*/) = 0;

        /**
         * Destroys an entire dynamic slice instance given the ID of any entity within the slice.
         * @param id The ID of the entity whose dynamic slice instance you want to destroy.
         * @return True if the dynamic slice instance was successfully destroyed. Otherwise, false.
         */
        virtual bool DestroyDynamicSliceByEntity(const AZ::EntityId& /*id*/) = 0;

        /**
        * Instantiates a dynamic slice asynchronously.
        * @param sliceAsset A reference to the slice asset data.
        * @param worldTransform A reference to the world transform to apply to the slice.
        * @return A ticket that identifies the slice instantiation request. Callers can immediately
        * subscribe to the AzFramework::SliceInstantiationResultBus for this ticket to receive results
        * for this request.
        */
        virtual SliceInstantiationTicket InstantiateDynamicSliceForBehaviorContext(const AZ::Data::Asset<AZ::Data::AssetData>& /*sliceAsset*/, const AZ::Transform& /*worldTransform*/) = 0;
        
        /**
         * Instantiates a dynamic slice asynchronously.
         * @param sliceAsset A reference to the slice asset data.
         * @param worldTransform A reference to the world transform to apply to the slice. 
         * @param customIdMapper An ID mapping function that is used when instantiating the slice.
         * @return A ticket that identifies the slice instantiation request. Callers can immediately 
         * subscribe to the AzFramework::SliceInstantiationResultBus for this ticket to receive results 
         * for this request. 
         */
        virtual SliceInstantiationTicket InstantiateDynamicSlice(const AZ::Data::Asset<AZ::Data::AssetData>& /*sliceAsset*/, const AZ::Transform& /*worldTransform*/, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& /*customIdMapper*/) = 0;

        /**
         * Cancels the asynchronous instantiation of a dynamic slice.
         * This call has no effect if the slice has already finished instantiation.
         * @param ticket The ticket that identifies the slice instantiation request.
         */
        virtual void CancelDynamicSliceInstantiation(const SliceInstantiationTicket& /*ticket*/) = 0;

        /**
         * Loads game entities from a stream.
         * @param stream The root slice.
         * @param remapIds Use true to remap the entity IDs after the stream is loaded.
         * @return True if the stream successfully loaded. Otherwise, false. This operation  
         * can fail if the source file is corrupt or the data could not be up-converted.
         */
        virtual bool LoadFromStream(AZ::IO::GenericStream& /*stream*/, bool /*remapIds*/) = 0;

        /**
         * Completely resets the game context. 
         * This includes deleting all slices and entities.
         */
        virtual void ResetGameContext() = 0;

        /**
         * Specifies that a given entity should not be activated by default 
         * after it is created.
         * @param entityId The entity that should not be activated by default.
         */
        virtual void MarkEntityForNoActivation(AZ::EntityId /*entityId*/) = 0;

        /**
         * Returns the entity's name.
         * @param id The ID of the entity.
         * @return The name of the entity. Returns an empty string if the entity 
         * cannot be found.
         */
        virtual AZStd::string GetEntityName(const AZ::EntityId&) = 0;

        /**
        * Find an entity in the game context.
        * @param entityId The ID of the entity.
        * @return A pointer to the entity.
        */
        virtual AZ::Entity* FindGameEntityById(AZ::EntityId /*entityId*/) = 0;

        /**
        * Find an entity in the game context.
        * @param entityId The ID of the entity.
        * @return An entity wrapper for use within the BehaviorContext.
        */
        virtual BehaviorEntity FindGameEntityByIdForBehaviorContext(AZ::EntityId /*entityId*/) = 0;

        /**
        * Find an entity in the game context.
        * @param name A name of the entity.
        * @return A pointer to the entity.
        */
        virtual AZ::Entity* FindGameEntityByName(const char* /*name*/) = 0;

        /**
        * Find an entity in the game context.
        * @param name A name of the entity.
        * @return An entity wrapper for use within the BehaviorContext.
        */
        virtual BehaviorEntity FindGameEntityByNameForBehaviorContext(const char* /*name*/) = 0;
    };

    /**
     * The EBus for requests to the game entity context.
     * The events are defined in the AzFramework::GameEntityContextRequests class.
     */
    using GameEntityContextRequestBus = AZ::EBus<GameEntityContextRequests>;

     /**
      * Interface for the AzFramework::GameEntityContextEventBus, which is the EBus 
      * that dispatches notification events from the game entity context. 
      * The game entity context holds gameplay entities, as opposed
      * to system entities, editor entities, and so on.
      */
    class GameEntityContextEvents
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~GameEntityContextEvents() = default;

        /**
         * Signals that the game entity context is loaded and activated, which happens at the  
         * start of a level. If the concept of levels is eradicated, this event will be removed.
         */
        virtual void OnGameEntitiesStarted() {}

        /**
         * Signals that the game entity context is shut down or reset. 
         * This is equivalent to the end of a level. 
         * This event will be valid even if the concept of levels is eradicated. 
         * In that case, its meaning will vary depending on how and if the game 
         * uses the game entity context.
         */
        virtual void OnGameEntitiesReset() {}

        /**
         * Signals that a slice was instantiated successfully.
         * @param sliceAssetId The asset ID of the slice to instantiate.
         * @param instance The slice instance.
         * @param ticket A ticket that identifies the slice instantiation request.
         */
        virtual void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*instance*/, const SliceInstantiationTicket& /*ticket*/) {}

        /**
         * Signals that a slice asset could not be instantiated.
         * @param sliceAssetId The asset ID of the slice that failed to instantiate.
         * @param ticket A ticket that identifies the slice instantiation request.
         */
        virtual void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/, const SliceInstantiationTicket& /*ticket*/) {}
    };

    /**
     * The EBus for game entity context events.
     * The events are defined in the AzFramework::GameEntityContextEvents class.
     */
    using GameEntityContextEventBus = AZ::EBus<GameEntityContextEvents>;

} // namespace AZ

#endif // AZCORE_GAMEENTITYCONTEXTBUS_H
