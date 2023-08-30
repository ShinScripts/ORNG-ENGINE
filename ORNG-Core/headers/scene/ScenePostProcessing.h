#pragma once

class FastNoiseSIMD;

namespace ORNG {
	class Texture3D;

	struct GlobalFog {
		void SetNoise(FastNoiseSIMD* p_noise);

		std::unique_ptr<Texture3D> fog_noise = nullptr;;
		glm::vec3 color{ 1 };
		float scattering_coef = 0.04f;
		float absorption_coef = 0.003f;
		float density_coef = 0.0f;
		int step_count = 32;
		float emissive_factor = 0.5f;
		float scattering_anistropy = 0.85f;
	};

	struct Bloom {
		float threshold = 1.0;
		float knee = 0.1;
		float intensity = 1.0;
	};


	struct PostProcessing {
		Bloom bloom;
		GlobalFog global_fog;
	};
}