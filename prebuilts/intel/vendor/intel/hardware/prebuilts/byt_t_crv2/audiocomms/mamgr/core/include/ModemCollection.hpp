/* @licence
 * INTEL CONFIDENTIAL
 * Copyright  2014 Intel
 * Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material contains trade secrets and proprietary
 * and confidential information of Intel or its suppliers and licensors. The
 * Material is protected by worldwide copyright and trade secret laws and
 * treaty provisions. No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or
 * disclosed in any way without Intels prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */
#pragma once

#include "Providers.hpp"
#include "Mutex.hpp"
#include <string>

namespace audio_comms
{
namespace mamgr
{

/** Class that contain multiple modems.
 *
 * Each modem can be adress given it's name and the path to the library
 * implementing it.
 *
 * As modems are unique, this class is a singleton.
 * Ie. If a modem is queried twice, the same instance will be returned in both
 *     cases.
 *
 * Libraries that contain a modem family implementation must fulfill the
 * Providers requirement. @see CreatorProvider
 */
class ModemCollection
{
private:
    typedef NInterfaceProvider::Providers<std::string> Providers;

public:
    /** Type of a modem,  */
    typedef NInterfaceProvider::IInterfaceProvider Modem;

    /** Helper to create a dynamic lib fulfilling the Providers requirement.
     *
     * The following code will declare the appropriate function to support
     * the MyModem modem type.
     *     extern "C" NInterfaceProvider::IInterfaceProvider *getInterfaceProvider()
     *     {
     *         static ModemCollection::CreatorProvider<MyModem> creatorProvider;
     *         return CretorProvider;
     *     }
     */
    template <class ModemProvider>
    struct CreatorProvider : public NInterfaceProvider::IInterfaceProvider,
                             private Providers::Creator
    {
        /** Return the only supported interface: Creator */
        virtual std::string getInterfaceList() const
        {
            return getInterfaceName();
        }

        /** Return the creator interface if queried or null.*/
        virtual IInterface *queryInterface(const std::string &name) const
        {
            if (name != getInterfaceName()) {
                return NULL;
            }

            /* Interface provider REQUIRES queryInterface to be const.
             * As it returns a non const member,
             * the only way to achieve that is by:
             *  - handle the interface through a pointer
             *    => pointer const but pointee not const
             *    (this trick is used in the interfaceProviderImpl)
             *  - Use a const cast to explicitly remove the cast
             *  @fixme Remove queryInterface constness and remove this const_cast
             */
            const IInterface *creator = this;
            return const_cast<IInterface *>(creator);
        }

        virtual NInterfaceProvider::IInterfaceProvider *create(std::string name)
        {
            return ModemProvider::createInstance(name);
        }
    };

    /** Return the singleton modem collection. */
    static ModemCollection &getInstance();

    /** Get a modem from the collection in a thread safe way. */
    Modem *getModem(const std::string &libraryPath, const std::string &modemName);

private:
    ModemCollection() {}

    /** Protect the providers against concurrent access. */
    utilities::Mutex providerLock;
    Providers providers;
};

} // namespace mamgr
} // namespace audio_comms
