#include "pch/pch.h"
#include "physics/Physics.h"
#include "extensions/PxDefaultSimulationFilterShader.h"

namespace ORNG {
	using namespace physx;
	void Physics::IShutdown() {
		mp_physics->release();
		mp_foundation->release();
		mp_cuda_context_manager->release();
	}

	void Physics::I_Init() {
		mp_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, g_default_allocator_callback, g_default_error_callback);

		if (!mp_foundation) {
			ORNG_CORE_CRITICAL("PxCreateFoundation failed");
			BREAKPOINT;
		}


		bool record_memory_allocations = true;



		mp_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *mp_foundation, PxTolerancesScale(), record_memory_allocations, mp_pvd);

		if (!mp_physics) {
			ORNG_CORE_CRITICAL("PxCreatePhysics failed");
			BREAKPOINT;
		}

		mp_cooking = PxCreateCooking(PX_PHYSICS_VERSION, *mp_foundation, PxCookingParams(m_tolerances_scale));
		if (!mp_cooking) {
			ORNG_CORE_CRITICAL("PxCreateCooking failed");
			BREAKPOINT;
		}

		const PxU32 num_threads = 8;
		mp_dispatcher = PxDefaultCpuDispatcherCreate(num_threads);
		/*if (!PxInitExtensions(*m_physics, m_pvd)) {
			ORNG_CORE_CRITICAL("PxInitExtensions failed");
			BREAKPOINT;
		}*/

		PxCudaContextManagerDesc cudaContextManagerDesc;

		mp_cuda_context_manager = PxCreateCudaContextManager(*Physics::GetFoundation(), cudaContextManagerDesc, PxGetProfilerCallback());


	}







}