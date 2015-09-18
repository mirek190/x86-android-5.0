/*
 * @file
 * Dynamic Library wrapper.
 *
 * @section License
 *
 * Copyright 2013-2014 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <NonCopyable.hpp>

#include <dlfcn.h>

namespace audio_comms
{

namespace utilities
{

/**
 * The DynamicLib class provides a suitable wrapper to load dynamic libraries in
 * C++ code. This class is NOT thread safe.
 */
class DynamicLib : private NonCopyable
{
public:
    /**
     * Loading mode for the library.
     */
    enum LoadMode
    {
        Lazy, /*< Load symbols on demand */
        Now   /*< Load all symbols now */
    };

    /**
     * DynamicLib constructor
     *
     * @param[in] libName    name of the library to load
     * @param[in] mode       symbol loading fashion. By default this is lazy.
     */
    explicit DynamicLib(const char *libName, LoadMode mode = Lazy)
        : _handle(NULL), _error()
    {
        /* Cleanup the error if any remains */
        dlerror();

        _handle = dlopen(libName, mode == Lazy ? RTLD_LAZY : RTLD_NOW);
        if (_handle == NULL) {
            /* Save the error */
            _error = dlerror();
        }
    }

    ~DynamicLib()
    {
        if (_handle != NULL) {
            dlclose(_handle);
            _handle = NULL;
        }
    }

    /**
     * Get a symbol of a given name and of given type in the dynamic lib This
     * function tries to retrieve a symbol of a given name inside the lib held
     * by the dynamic class. It the symbol is found, it is returned casted to
     * the type given as template parameter.
     * Careful: This function do not perform any check on the type validity,
     * this means that this is the caller responsibility to make sure the
     * symbol will fit the type. If symbol is not found, or lib not correctly
     * loaded, this function returns NULL and the caller may retrieve the error
     * description by calling getError().
     *
     * @tparam SymType     Type of the symbol to retrieve
     * @param[in] symName  The symbol to fetch
     *
     * @return the symbol with the type passed as template or NULL if not found.
     */
    template <class SymType>
    SymType *getSymbol(const char *symName) const
    {
        if (_handle == NULL) {
            return NULL;
        }

        /* reset any remaining error */
        dlerror();

        /* No special care on the symName validity, if symbol is not found,
         * dlsym will fail anyway. */
        SymType *sym = reinterpret_cast<SymType *>(dlsym(_handle, symName));
        if (sym == NULL) {
            _error = dlerror();
        }
        return sym;
    }

    /**
     * get the last error that the lib encountered.
     *
     * @return the last error.
     */
    std::string &getError() const
    {
        return _error;
    }

private:
    void *_handle;              /*< handle on the open dynamic library */

    /** Error string is made mutable because I want to keep the getSymbol()
     * function to be const as user usually expect the getters to be const. */
    mutable std::string _error; /*< Description of the error, is any occurred. */
};

}

}
