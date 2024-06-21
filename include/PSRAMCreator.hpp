#ifndef ENERGYLEAF_SENSOR_PSRAMCREATOR_HPP 
#define ENERGYLEAF_SENSOR_PSRAMCREATOR_HPP

#include "Extras/Memory/ICreator.hpp"

template <class Type>
class PSRAMCreator : public Apalinea::Extras::Memory::ICreator<Type> {
    public:
    Type* create(std::size_t size) override {
        auto pointer = static_cast<Type*>(ps_malloc(size));
        if (pointer) {
            return pointer;
        }
        throw std::bad_alloc();
    }

    void destroy(Type* ptr, Apalinea::Extras::Memory::CreatorArgument arg = Apalinea::Extras::Memory::CreatorArgument::SINGLE) override {
        free(ptr);
    }
};

#endif //ENERGYLEAF_SENSOR_PSRAMCREATOR_HPP