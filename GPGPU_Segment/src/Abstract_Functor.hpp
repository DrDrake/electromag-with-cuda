/*
 * Copyright (C) 2010 - Alexandru Gagniuc - <mr.nuke.me@gmail.com>
 * This file is part of ElectroMag.
 *
 * ElectroMag is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ElectroMag is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 *  along with ElectroMag.  If not, see <http://www.gnu.org/licenses/>.
 */


/** ============================================================================
 * \defgroup DEVICE_FUNCTORS Device Functors
 *
 * @{
 * ===========================================================================*/

#ifndef _ABSTRACT_FUNCTOR_H
#define _ABSTRACT_FUNCTOR_H
#include <mutex>
#include <Data Structures.h>


/** ****************************************************************************
 * \brief Abstract class for standardizing functor operation
 *
 * A functor should be interpreted as a set of functions that performs
 * a given task or se of tasks, in this context, a specific set of calculations.
 * \n \n
 * There are a number of considerations to take into account for a funnctor.
 * It should organize the given data into whatever format makes more sense for
 * for the given compute device or devices, and the functionality it implements.
 * Alhough the data format and the functionality are dependent on the specific
 * functionality the functor provides, some aspects are common to all functors:
 * \n
 * 1) The interface to the outside world \n
 * 2) The assignment and of threads that execute the given task. Normally, when
 * several compute devices are used by the functor, it makes most sense to
 * assign a separate controlling thread for each device. Also, if a device, for
 * whatever reason, fails to complete its part task, then it should be
 * reassigned to a device that has succesfully terminated, and still has
 * resources allocated. These points are implemented in the AbstractFunctor
 * class, which allows derived classes to focus on device specific functionality
 * \n \n
 * A TODO point is to allow several functors to share data, or even complete
 * more complex tasks with the same given set of resources.
 */
class AbstractFunctor
{
public:

    AbstractFunctor();
    virtual ~AbstractFunctor();

    struct AbstractFunctorParams
    {
    };

    /**
     * \brief Runs the calculations
     * 
     * Once the functor has been fully "initialized", with resources allocated
     * for each device the calculations may be started. \n
     * This functinality is provided by the Run function. 
     */
    virtual unsigned long Run();

    ///\brief Binds a dataset to the object
    virtual void BindData(void *dataParameters) = 0;

    /// \brief Resource allocation and deallocation functions
    ///{@
    virtual void AllocateResources() = 0;
    virtual void ReleaseResources() = 0;
    ///@}

    /**
     * \brief Generates a list of 'nDevices' parameters to be passed to
     * \brief 'nDevices' functors
     * 
     * This function decides how the data gets split among different devices,
     * and generates the appropeiate parameter list. The paremeters created by
     * this routine are passed to the main functor in a multithreaded manner
     */
    virtual void GenerateParameterList(size_t *nDevices) = 0;

    /**
     * \brief Main Functor: runs the data belonging tu functor 'functorIndex' on
     * \brief device 'deviceID'
     * 
     * Under normal circumstances, functorIndex and deviceIndex should be equal,
     * however, if execution of part of the data failed on deviceIndex, it will
     * be remapped to the first idle device. The derived implementation must
     * make sure that any functor can run on any devce, even if at reduced
     * performance.
     */
    virtual unsigned long MainFunctor(
        size_t functorIndex, size_t deviceIndex
        ) = 0;

    /**
     * \brief Auxiliary Functor: runs concurrently with the main functor in a
     * \brief separate thread
     * 
     * This function is called after creating the worker threads for the main
     * functor. If this function does not return after the other worker threads
     * terminate, its thread will be killed. The auxiliary functor should thus
     * not be used for any critical purpose, as it may be unexpectedly
     * terminated. It can however be used for monitoring the status of the
     * worker functors, and/or combining real-time performance and progress
     * information
     */
    virtual unsigned long AuxFunctor() = 0;

    /**
     * \brief Does any necessarry tasks after all functors have completed
     * \brief execution
     */
    virtual void PostRun() = 0;

    /**
     * \brief Checks for errors on global operations
     * 
     * Returns true if the previous global operation has failed
     */
    virtual bool Fail() = 0;
    /**
     * \brief Checks for errors on operations performed on a specific device
     * \brief functor
     * 
     * Returns true if the previous operation on the given functor has failed.
     * Also returns true if the functor is out of bounds. \n
     * This function is NOT required to be thread-safe. Its implementation
     * should be thread-safe if the derived class uses it in a way that requires
     * thread-safety. The Abstract functor uses it sequentially with operations
     * on the device functor.
     */
    virtual bool FailOnFunctor(size_t functorIndex) = 0;

private:
    //--------------------Functor Remapping Constructs------------------------//
    struct AsyncParameters
    {
        /// Pointer to an AbstractFunctor object that is ready to call ts main
        /// functor
        AbstractFunctor* functorClass;
        // This should be identical cross all calls of AsyncFunctor in Run()
        /// Number of functors the object has split the data ib
        size_t nFunctors;
        /// The index of the functor on which to run the calculations
        size_t functorIndex;
    };
    /// Mutex used for syncronization when remapping failed functors
    std::mutex hRemapMutex;

    size_t * idleDevices;
    size_t * failedFunctors;
    size_t nFailed;
    size_t nIdle;
    
    /**
     * \brief Calls the main functor statically, Then handles remapping
     * 
     * This also takes care of remapping failed threads to succesfull threads.
     * There is an implicit assumption of homogenuity between devices (threads),
     * but the derived class can detect a remap, and should compensate for such
     * inhomogenuities, should they occur.
     * \n
     * This function is also used as the thread entry point.
     */
    static unsigned long AsyncFunctor(AsyncParameters *parameters);
    static unsigned long AsyncAuxFunctor(AsyncParameters *parameters);



};

/** ============================================================================
 * @}
 * ===========================================================================*/




#endif//_ABSTRACT_FUNCTOR_H
