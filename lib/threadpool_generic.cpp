/** @file lib/threadpool_generic.cpp
 *
 * Threadpool for C++11, library of generic thread pool
 *
 * @copyright	2014 Ruediger Helsch, Ruediger.Helsch@t-online.de
 * @license	All rights reserved. Use however you want. No warranty at all.
 * $Revision: 1.5 $
 * $Date: 2014/05/14 14:06:42 $
 */
#define THREADPOOL_USE_LIBRARY 1

#include "threadpool/impl/threadpool_generic_inst.h"

namespace ThreadPoolImpl {
    namespace impl {

	template class GenericThreadPoolTmpl<>;

    }
}
