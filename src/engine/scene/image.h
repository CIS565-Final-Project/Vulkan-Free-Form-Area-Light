#pragma once

namespace VK_Renderer
{
	class Image
	{
	public:
		virtual ~Image();

		virtual void* GetRawData() const { return m_RawData; }
		virtual void LoadFromFile(std::string const& file);
		virtual uint32_t GetSize() const { return m_Size; }
	protected:
		void* m_RawData{ nullptr };
		uint32_t m_Size{ 0 };
		DeclareWithGetFunc(protected, glm::ivec3, m, Resolution, const);
	};
}