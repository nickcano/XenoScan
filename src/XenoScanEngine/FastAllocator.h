#pragma once

#include <list>
#include <memory>
#include <stdint.h>
#include <iostream>

/*
	This is a very very fast block-based allocator that is used for the scan result list.
	It has roughly ~250x performance over the standard allocator used by STL containers, 
	but is is memory-inefficient. Additionally, it breaks the .swap() functionality of stl
	containers.

	This is a small trade-off for the performance gains and it's ability to clear up allocated
	memory when it is destroyed.
*/

template<size_t bytesPerBlock>
struct FastAllocatorBlock
{
	FastAllocatorBlock() : index(0) {}
	size_t index;
	uint8_t objects[bytesPerBlock];

	template<typename T, typename size_type, typename pointer_type>
	inline bool allocateItems(const typename std::allocator<T>::size_type n, pointer_type *destination)
	{
		if (this->index + (n * sizeof(T)) < bytesPerBlock)
		{
			*destination = (pointer_type)&this->objects[this->index];
			this->index += (n * sizeof(T));
			return true;
		}
		else
			return false;
	}
};

template<typename T>
class FastAllocator
{
public:
	static const size_t     bytesPerBlock = 1000000;
	typedef T               value_type;
	typedef T*              pointer;
	typedef T&              reference;
	typedef const T*        const_pointer;
	typedef const T&        const_reference;
	typedef size_t          size_type;
	typedef ptrdiff_t       difference_type;

	inline pointer address(reference x) const throw() { return &x; }
	inline const_pointer address(const_reference x) const throw() { return &x; }
	inline void construct(pointer p, const_reference val) { new (p) value_type (val); }
	inline void destroy(pointer p) { p->~value_type(); }
	inline bool operator==(FastAllocator const& that) { return this->blocks.get() == that.blocks.get(); }
	inline bool operator!=(FastAllocator const& a) { return !operator==(a); }

	inline size_type max_size() const throw()
	{
		return bytesPerBlock * 100000;
	}

	inline FastAllocator()
	{
		this->blocks = std::make_shared<std::list<FastAllocatorBlock<bytesPerBlock>>>();
		this->blocks->push_front(FastAllocatorBlock<bytesPerBlock>());
		std::cout << "    New FastAllocator " << this << " blocks " << this->blocks.get() << std::endl;
	}

	inline FastAllocator(const FastAllocator& that) throw()
	{
		this->blocks = that.blocks;
		std::cout << "    Copy FastAllocator " << this << " blocks " << this->blocks.get() << std::endl;
	}

	template <typename U>
	inline FastAllocator(const FastAllocator<U>& that) throw()
	{
		this->blocks = that.blocks;
		std::cout << "    Convert FastAllocator " << this << " blocks " << this->blocks.get() << std::endl;
	}

	inline virtual ~FastAllocator()
	{
		std::cout << "    Remove FastAllocator " << this << " blocks " << this->blocks.get() << std::endl;
	}

	inline pointer allocate(const size_type n, const_pointer = 0)
	{
		pointer ret;
		if (!this->blocks->begin()->allocateItems<T, size_type, pointer>(n, &ret))
		{
			this->blocks->push_front(FastAllocatorBlock<bytesPerBlock>());
			if (!this->blocks->begin()->allocateItems<T, size_type, pointer>(n, &ret))
			{
				//std::cout << "    Allocate " << n << " objects failed" << std::endl;
				// we're trying to allocate more than a single block can handle..
				// need to come up with a case for this
			}
		}
		return ret;
	}

	inline void deallocate(const typename pointer p, const typename size_type n)
	{
		// do nothing
	}

	template<typename U>
	struct rebind
	{
		typedef FastAllocator<U> other;
	};

	std::shared_ptr<std::list<FastAllocatorBlock<bytesPerBlock>>> blocks;
};