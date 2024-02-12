//
// Created by SlepiK on 01.02.24.
//

#ifndef ENERGYLEAF_STREAM_V1_EXPTRAS_MEMORY_PSRAMCREATOR_HPP 
#define ENERGYLEAF_STREAM_V1_EXPTRAS_MEMORY_PSRAMCREATOR_HPP

#include <Extras/Memory/ICreator.hpp>

    template <class Type>
    class PSRAMCreator : public Energyleaf::Stream::V1::Extras::Memory::ICreator<Type> {
        public:
        Type* create(std::size_t size) override {
            auto pointer = static_cast<Type*>(ps_malloc(size));
            if (pointer) {
                return pointer;
            }
            throw std::bad_alloc();
        }

        void destroy(Type* ptr, Energyleaf::Stream::V1::Extras::Memory::CreatorArgument arg = Energyleaf::Stream::V1::Extras::Memory::CreatorArgument::SINGLE) override {
            free(ptr);
        }
    };

#endif