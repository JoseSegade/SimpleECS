#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <assert.h>
#include <typeinfo>
#include <set>
#include <memory>

namespace ECS
{
    using Entity = std::uint32_t;

    static const Entity INVALID = 0;
    static std::atomic<Entity> next{INVALID + 1};

    Entity CreateEntity()
    {
        return next.fetch_add(1);
    }

    class IComponentArray
    {
    public:
        virtual ~IComponentArray() = default;

        virtual bool Contains(Entity entity) = 0;
        virtual void OnEntityDestroyed(Entity entity) = 0;
        virtual std::vector<Entity> &Entities() = 0;
        virtual void Create(Entity entity) = 0;
    };

    template <typename T>
    class ComponentArray : public IComponentArray
    {
    public:
        inline ComponentArray(size_t maxComponents = 0)
        {
            mComponents.reserve(maxComponents);
            mEntities.reserve(maxComponents);
            mEntityToIndexMap.reserve(maxComponents);
        }

        inline void Create(Entity entity) override
        {
            assert(entity != INVALID_ENTITY);
            assert(mEntityToIndexMap.count(entity) == 0);
            assert(mEntities.size() == mComponents.size());
            assert(mEntityToIndexMap.size() == mComponents.size());

            mEntityToIndexMap[entity] = mComponents.size();

            mComponents.emplace_back();
            mEntities.push_back(entity);

            mComponents.back();
        }

        inline void Remove(Entity entity)
        {
            assert(entity != INVALID_ENTITY);
            assert(mEntityToIndexMap.count(entity) > 0);

            size_t indexRemoved = mEntityToIndexMap.at(entity);

            if (indexRemoved < mComponents.size() - 1)
            {
                mComponents[indexRemoved] = std::move(mComponents.back());
                mEntities[indexRemoved] = mEntities.back();

                mEntityToIndexMap[mEntities.at(indexRemoved)] = indexRemoved;
            }

            mComponents.pop_back();
            mEntities.pop_back();
            mEntityToIndexMap.erase(entity);
        }

        bool Contains(Entity entity) override
        {
            return mEntityToIndexMap.count(entity) > 0;
        }

        T &Get(Entity entity)
        {
            assert(mEntityToIndexMap.count(entity) > 0);

            return mComponents[mEntityToIndexMap.at(entity)];
        }

        inline void OnEntityDestroyed(Entity entity) override
        {
            if (mEntityToIndexMap.count(entity) > 0)
            {
                Remove(entity);
            }
        }

        inline std::vector<Entity> &Entities() override
        {
            return mEntities;
        }

    private:
        std::vector<T> mComponents;

        std::vector<Entity> mEntities;
        std::unordered_map<Entity, size_t> mEntityToIndexMap;

        size_t mSize;
    };

    class Register;
    class System
    {
    public:
        System() = default;
        virtual ~System() = default;

        virtual void Init(){};
        virtual void Update(double dt){};

        virtual void SetRegister(Register *thatRegister) = 0;
        virtual void AddEntity(Entity entity) = 0;
        virtual void RemoveEntity(Entity entity) = 0;
        virtual std::vector<std::string> GetComponentDependencies() = 0;
        virtual void AddEntities(const std::vector<Entity> &entities) = 0;
    };
    template <typename... Cs>
    class ECSSystem : public System
    {
    public:
        ECSSystem() = default;
        virtual ~ECSSystem() = default;

        inline void AddEntity(Entity entity) override
        {
            mEntities.insert(entity);
        };

        inline void RemoveEntity(Entity entity) override
        {
            mEntities.erase(entity);
        }

        inline std::vector<std::string> GetComponentDependencies()
        {
            return {typeid(Cs).name()...};
        }

        inline void AddEntities(const std::vector<Entity> &entities)
        {
            std::copy(entities.begin(), entities.end(), std::inserter(mEntities, mEntities.end()));
        }

        inline void SetRegister(Register *thatRegister)
        {
            mRegister = thatRegister;
        }

    protected:
        std::set<Entity> mEntities;
        Register *mRegister;
    };

    class Register
    {
    public:
        Register() = default;

        inline void ClearEntity(Entity entity)
        {
            for (auto &[_, components] : mComponents)
            {
                components->OnEntityDestroyed(entity);
            }
            for (auto &[_, system] : mSystems)
            {
                system->RemoveEntity(entity);
            }
        }

        template <typename C>
        inline void RegisterComponent()
        {
            std::string componentName = typeid(C).name();
            auto c = std::make_unique<ComponentArray<C>>();
            mComponents.emplace(componentName, std::move(c));
        }

        template <typename... Cs>
        inline void AddComponents(Entity entity)
        {
            std::vector<std::string> componentNames = {typeid(Cs).name()...};
            for (const std::string &componentName : componentNames)
            {
                auto it = mComponents.find(componentName);
                if (it != mComponents.end())
                {
                    it->second->Create(entity);
                }
            }
        }

        template <typename C>
        inline void RemoveComponent(Entity entity)
        {
            std::string componentName = typeid(C).name();
            assert(mComponents.count(componentName) > 0);

            mComponents.at(componentName)->OnEntityDestroyed(entity);
        }

        template <typename C>
        inline C &GetComponent(Entity entity)
        {
            return GetAsComponentArray<C>()->Get(entity);
        }

        template <class T>
        inline System *RegisterSystem()
        {
            std::string systemName = typeid(T).name();

            mSystems.emplace(systemName, std::make_unique<T>());
            auto &sys = mSystems.at(systemName);

            sys->SetRegister(this);
            auto dependencies = sys->GetComponentDependencies();

            for (const std::string &dep : dependencies)
            {
                sys->AddEntities(mComponents.at(dep)->Entities());
            }

            return mSystems.at(systemName).get();
        }

    private:
        template <typename C>
        inline ComponentArray<C> *GetAsComponentArray()
        {
            const std::string componentName = typeid(C).name();
            assert(mComponents.count(componentName) > 0);

            return reinterpret_cast<ComponentArray<C> *>(mComponents.at(componentName).get());
        }

        std::unordered_map<std::string, std::unique_ptr<IComponentArray>> mComponents{};
        std::unordered_map<std::string, std::unique_ptr<System>> mSystems{};
    };
}