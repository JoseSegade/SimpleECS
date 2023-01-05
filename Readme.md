# Simple Entity Component System

Simple file implementing an Entity Component System for fast prototyping. Written in C++20.

## Functions & Features

- `Entity` is just an identifier.
- `Component` could be any kind of data.
- `System` functions *:
  - `Init()`.
  - `Update(double dt)`.

^* Note custom systems must inherit from `ECSSystem` to specify the dependencies.

- `Register` functions:
  - `AddComponents<...components>(Entity)`: binds new component/s of the specified type to an existing entity. Cannot add an unregister component.
  - `RegisterComponent<component>()`: stores component types.
  - `RemoveComponent<component>(Entity)`: removes an existing component from an entity. Also removes the entity from all systems using it.
  - `GetComponent<component>(Entity)`: obtains the component object from an entity. Entity must have a component binded previously.
  - `RegisterSystem<System>()`: creates a new system. When a system is registered will consume **ONLY** the entities binded with at least one component marked as dependency in `ECSSystem` class header.

## Usage example:

```Cpp
#include <cstdio>
#include <SimpleECS.h>

struct PositionComponent
{
   float x = 0.0f;
   float y = 0.0f;
   float z = 0.0f;
};

struct NameComponent
{
   std::string name = "";
};

class MySystem : public ECS::ECSSystem<PositionComponent, NameComponent>
{
public:
   MySystem() = default;

   void Init() override
   {
      int i = 0;
      for(auto entity : mEntities)
      {
         mRegister->GetComponent<NameComponent>(entity).name = "Entity_" + std::to_string(i);

         ++i;
      }
   }

   void Update(double dt) override
   {
      for(auto entity : mEntities)
      {
         auto& pos = mRegister->GetComponent<PositionComponent>(entity);
         pos.x += 2.0;

         printf("%s[%f, %f, %f]\n", mRegister->GetComponent<NameComponent>(entity).name.c_str(), pos.x, pos.y, pos.z);
      }
   }

};

int main()
{
   ECS::Register reg{};

   ECS::Entity entity0 = ECS::CreateEntity();
   ECS::Entity entity1 = ECS::CreateEntity();

   reg.RegisterComponent<PositionComponent>();   
   reg.RegisterComponent<NameComponent>();

   reg.AddComponents<PositionComponent, NameComponent>(entity0);
   reg.AddComponents<PositionComponent, NameComponent>(entity1);
   
   auto system = reg.RegisterSystem<MySystem>();

   system->Init();
   for(double i = 0.0; i < 25.0; i+=1.0)
   {
      system->Update(static_cast<double>(i));
   }


   return 0;
}
```


