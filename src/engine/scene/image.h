#pragma once

namespace VK_Renderer
{
	class Image
	{
	public:
		virtual ~Image();

		virtual void* GetRawData() const { return m_RawData; }

		virtual void LoadFromFile(std::string const& file);
		virtual uint32_t GetSize() const { return m_Resolution.x * m_Resolution.y * 4 * sizeof(unsigned char); }
	protected:
		void* m_RawData{ nullptr };
		DeclareWithGetFunc(protected, glm::ivec3, m, Resolution, const);
	};
}