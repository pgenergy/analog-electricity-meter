//
// Created by Simon Stahmer on 29.01.24.
//

#ifndef ENERGYLEAF_STREAM_V1_TYPES_EMPTYTYPE_HPP
#define ENERGYLEAF_STREAM_V1_TYPES_EMPTYTYPE_HPP

namespace Energyleaf::Stream::V1::Types {
    /**
     * This class represents a empty object or empty type, because void as type is not direct allowed in this library!
     */
    class Empty {
    public:
        Empty() {
        }
        
        virtual ~Empty() = default;

        Empty(Empty &&other){
        }

        Empty(const Empty& other) {
        }

        Empty& operator=(Empty&& other) {
            return *this;
        }

        Empty& operator=(const Empty& other) {
            return *this;
        }
    };
}

#endif //ENERGYLEAF_STREAM_V1_TYPES_EMPTYTYPE_HPP
