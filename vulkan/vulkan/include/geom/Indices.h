#ifndef _INDICES_H_
#define _INDICES_H_
#include <glm/glm.hpp>
#include <vector>
namespace geom
{
	class Indices
	{
	public:
		static const std::vector<uint16_t> MakeSquareIndices()
		{
			return { 0,1,2,2,3,0 };
		}
	private:
	};
}
#endif // !_INDICES_H_
