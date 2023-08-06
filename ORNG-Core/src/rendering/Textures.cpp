#include "pch/pch.h"

#include "rendering/Textures.h"
#include "util/Log.h"
#include "core/GLStateManager.h"

namespace ORNG {


	bool TextureBase::LoadFloatImageFile(const std::string& filepath, GLenum target, const TextureBaseSpec* base_spec, unsigned int layer) {

		stbi_set_flip_vertically_on_load(1);

		int width = 0;
		int	height = 0;
		int	bpp = 0;

		float* image_data = stbi_loadf(filepath.c_str(), &width, &height, &bpp, 0);

		if (image_data == nullptr) {
			ORNG_CORE_ERROR("Can't load texture from '{0}', - '{1}'", filepath.c_str(), stbi_failure_reason());
			return false;
		}

		GL_StateManager::BindTexture(m_texture_target, m_texture_obj, GL_TEXTURE0, true);
		GLenum internal_format;
		GLenum format;

		switch (bpp) {
		case 1:
			internal_format = GL_R32F;
			format = GL_RED;
			break;
		case 2:
			internal_format = GL_RG32F;
			format = GL_RG;
			break;
		case 3:
			internal_format = GL_RGB32F;
			format = GL_RGB;
			break;
		case 4:
			internal_format = GL_RGBA32F;
			format = GL_RGBA;
			break;
		default:
			ORNG_CORE_ERROR("Failed loading texture from '{0}', unsupported number of channels", filepath);
			stbi_image_free(image_data);
			return false;
		}

		glTexImage2D(target, 0, internal_format, width, height, 0, format, GL_FLOAT, image_data);

		stbi_image_free(image_data);

		if (base_spec->generate_mipmaps)
			glGenerateMipmap(m_texture_target);

		GL_StateManager::BindTexture(m_texture_target, 0, GL_TEXTURE0, true);

		return true;
	}

	bool TextureBase::LoadImageFile(const std::string& filepath, GLenum target, const TextureBaseSpec* base_spec, unsigned int layer) {

		stbi_set_flip_vertically_on_load(1);

		int width = 0;
		int	height = 0;
		int	bpp = 0;

		unsigned char* image_data = stbi_load(filepath.c_str(), &width, &height, &bpp, 0);

		if (image_data == nullptr) {
			ORNG_CORE_ERROR("Can't load texture from '{0}', - '{1}'", filepath.c_str(), stbi_failure_reason());
			return false;
		}

		GLenum internal_format;
		GLenum format;

		GL_StateManager::BindTexture(m_texture_target, m_texture_obj, GL_TEXTURE0, true);

		switch (bpp) {
		case 1:
			internal_format = GL_R8;
			format = GL_RED;
			break;
		case 2:
			internal_format = GL_RG8;
			format = GL_RG;
			break;
		case 3:
			if (base_spec->srgb_space)
				internal_format = GL_SRGB8;
			else
				internal_format = GL_RGB8;

			format = GL_RGB;
			break;
		case 4:
			if (base_spec->srgb_space)
				internal_format = GL_SRGB8_ALPHA8;
			else
				internal_format = GL_RGBA8;

			format = GL_RGBA;
			break;
		default:
			ORNG_CORE_ERROR("Failed loading texture from '{0}', unsupported number of channels", filepath);
			stbi_image_free(image_data);
			return false;
		}

		glTexImage2D(target, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, image_data);
		stbi_image_free(image_data);

		if (base_spec->generate_mipmaps)
			glGenerateMipmap(m_texture_target);
		GL_StateManager::BindTexture(m_texture_target, 0, GL_TEXTURE0, true);

		return true;
	}


	bool Texture2D::LoadFromFile() {

		if (!ValidateBaseSpec(static_cast<const TextureBaseSpec*>(&m_spec)) || m_spec.filepath.empty()) {
			ORNG_CORE_ERROR("2D Texture failed loading: Invalid spec");
			return false;
		}

		bool ret = true;

		if (m_spec.storage_type == GL_FLOAT)
			ret = LoadFloatImageFile(m_spec.filepath, GL_TEXTURE_2D, static_cast<TextureBaseSpec*>(&m_spec));
		else
			ret = LoadImageFile(m_spec.filepath, GL_TEXTURE_2D, static_cast<TextureBaseSpec*>(&m_spec));

		GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
		glTexParameteri(m_texture_target, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
		glTexParameteri(m_texture_target, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
		glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_S, wrap_mode);
		glTexParameteri(m_texture_target, GL_TEXTURE_WRAP_T, wrap_mode);


		return ret;
	}



	bool Texture2DArray::LoadFromFile() {

		if (!ValidateBaseSpec(static_cast<const TextureBaseSpec*>(&m_spec)) || m_spec.filepaths.empty()) {
			ORNG_CORE_ERROR("Texture2DArray failed loading: Invalid spec");
			return false;
		}

		bool ret = true;


		GL_StateManager::BindTexture(m_texture_target, m_texture_obj, GL_TEXTURE0, true);
		glTexImage3D(m_texture_target, 0, m_spec.internal_format, m_spec.width, m_spec.height, m_spec.filepaths.size(), 0, m_spec.format, m_spec.storage_type, nullptr);


		for (int i = 0; i < m_spec.filepaths.size(); i++) {

			int width = 0;
			int	height = 0;
			int	bpp = 0;

			unsigned char* image_data = stbi_load(m_spec.filepaths[i].c_str(), &width, &height, &bpp, 4);

			if (image_data == nullptr) {
				ORNG_CORE_ERROR("Can't load texture from '{0}', - '{1}'", m_spec.filepaths[i].c_str(), stbi_failure_reason());
				return false;
			}

			if (width != m_spec.width || height != m_spec.height) {
				ORNG_CORE_ERROR("2D Texture loading error: width/height mismatch");
				stbi_image_free(image_data);
				return false;
			}

			glTexSubImage3D(m_texture_target, 0, 0, 0, i, width, height, 1, m_spec.format, m_spec.storage_type, image_data);

			stbi_image_free(image_data);
		}


		GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, wrap_mode);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, wrap_mode);


		return true;
	}






	bool TextureCubemap::LoadFromFile() {

		if (!ValidateBaseSpec(static_cast<const TextureBaseSpec*>(&m_spec)) || m_spec.filepaths.size() != 6) {
			ORNG_CORE_ERROR("TextureCubemap failed to load, invalid spec");
			return false;
		}

		for (unsigned int i = 0; i < m_spec.filepaths.size(); i++) {

			bool ret = true;

			if (m_spec.storage_type == GL_FLOAT)
				ret = LoadFloatImageFile(m_spec.filepaths[i], GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, static_cast<TextureBaseSpec*>(&m_spec));
			else
				ret = LoadImageFile(m_spec.filepaths[i], GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, static_cast<TextureBaseSpec*>(&m_spec));

		}

		GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap_mode);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrap_mode);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap_mode);



		return true;
	}



	bool Texture2D::SetSpec(const Texture2DSpec& spec) {
		if (ValidateBaseSpec(static_cast<const TextureBaseSpec*>(&spec))) {
			m_spec = spec;
			GL_StateManager::BindTexture(GL_TEXTURE_2D, m_texture_obj, GL_TEXTURE0, true);

			GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);

			if (m_spec.internal_format != GL_NONE && m_spec.format != GL_NONE)
				glTexImage2D(GL_TEXTURE_2D, 0, spec.internal_format, spec.width, spec.height, 0, spec.format, spec.storage_type, nullptr);

			if (m_spec.generate_mipmaps)
				glGenerateMipmap(m_texture_target);

			return true;
		}
		else {
			ORNG_CORE_ERROR("Texture2D failed setting spec: Invalid spec");
			return false;
		}
	}

	bool Texture2DArray::SetSpec(const Texture2DArraySpec& spec) {
		if (ValidateBaseSpec(static_cast<const TextureBaseSpec*>(&spec))) {
			m_spec = spec;
			GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, m_texture_obj, GL_TEXTURE0, true);

			GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, wrap_mode);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, wrap_mode);

			if (m_spec.internal_format != GL_NONE && m_spec.format != GL_NONE)
				glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, m_spec.internal_format, m_spec.width, m_spec.height, m_spec.layer_count, 0, m_spec.format, m_spec.storage_type, nullptr);


			if (m_spec.generate_mipmaps)
				glGenerateMipmap(m_texture_target);
			return true;
		}
		else {
			ORNG_CORE_ERROR("Texture2DArray failed setting spec: Invalid spec");
			return false;
		}
	}

	bool Texture3D::SetSpec(const Texture3DSpec& spec) {
		if (ValidateBaseSpec(static_cast<const TextureBaseSpec*>(&spec))) {
			m_spec = spec;
			GL_StateManager::BindTexture(GL_TEXTURE_3D, m_texture_obj, GL_TEXTURE0, true);

			GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap_mode);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap_mode);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap_mode);

			if (m_spec.internal_format != GL_NONE && m_spec.format != GL_NONE)
				glTexImage3D(GL_TEXTURE_3D, 0, m_spec.internal_format, m_spec.width, m_spec.height, m_spec.layer_count, 0, m_spec.format, m_spec.storage_type, nullptr);


			if (m_spec.generate_mipmaps)
				glGenerateMipmap(m_texture_target);


			return true;
		}
		else {
			ORNG_CORE_ERROR("3D Texture failed setting spec: Invalid spec");
			return false;
		}
	}


	bool TextureCubemap::SetSpec(const TextureCubemapSpec& spec) {
		if (ValidateBaseSpec(static_cast<const TextureBaseSpec*>(&spec))) {
			ASSERT(m_spec.filepaths.size() == 6);
			m_spec = spec;
			GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, m_texture_obj, GL_TEXTURE0, true);


			ASSERT(m_spec.internal_format != GL_NONE && m_spec.format != GL_NONE);

			for (unsigned int i = 0; i < 6; i++) {
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, spec.internal_format, spec.width, spec.height, 0, spec.format, spec.storage_type, nullptr);
			}

			GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap_mode);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrap_mode);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap_mode);

			if (m_spec.generate_mipmaps)
				glGenerateMipmap(m_texture_target);
			return true;
		}
		else {
			ORNG_CORE_ERROR("TextureCubemap failed setting spec: Invalid spec");
			return false;
		}
	}

	bool TextureCubemapArray::SetSpec(const TextureCubemapArraySpec& spec) {
		if (ValidateBaseSpec(static_cast<const TextureCubemapArraySpec*>(&spec))) {
			m_spec = spec;
			GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_texture_obj, GL_TEXTURE0, true);


			ASSERT(m_spec.internal_format != GL_NONE && m_spec.format != GL_NONE);

			glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, spec.internal_format, spec.width, spec.height, spec.layer_count * 6, 0, spec.format, spec.storage_type, nullptr);

			GLenum wrap_mode = m_spec.wrap_params == GL_NONE ? GL_REPEAT : m_spec.wrap_params;
			glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, m_spec.min_filter == GL_NONE ? GL_NEAREST : m_spec.min_filter);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, m_spec.mag_filter == GL_NONE ? GL_NEAREST : m_spec.mag_filter);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, wrap_mode);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, wrap_mode);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, wrap_mode);

			if (m_spec.generate_mipmaps)
				glGenerateMipmap(m_texture_target);
			return true;
		}
		else {
			ORNG_CORE_ERROR("TextureCubemapArray failed setting spec: Invalid spec");
			return false;
		}
	}
}