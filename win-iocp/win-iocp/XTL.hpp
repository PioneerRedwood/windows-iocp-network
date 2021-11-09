/* 학습자 코멘트
직접 구현한 ObjectPool에 맞도록 STL처럼 컨테이너를 선언


*/
#pragma once
#include "MemoryPool.hpp"
#include <list>
#include <vector>
#include <deque>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <queue>

template<class T>
class STLAllocator
{
public:
	STLAllocator() = default;

	using value_type = T;
	using pointer = *value_type;
	using const_pointer = const* value_type;
	using reference = &value_type;
	using const_reference = &reference;
	using size_type = std::size_t;
	using different_type = std::ptrdiff_t;

	template<class U>
	STLAllocator(const STLAllocator<U>&)
	{}

	template<class U>
	struct rebind
	{
		using other = STLAllocator<U>;
	};

	void construct(pointer p, const_reference t)
	{
		new(p)T(t);
	}

	void destroy(pointer p)
	{
		p->~T();
	}

	T* allocate(size_t n)
	{
		// TODO: MemoryPool에서 할당해서 리턴
		return static_cast<pointer>(GMemoryPool->Allocate(n * sizeof(T)));
	}

	void deallocate(pointer p, size_t n)
	{
		// TODO: MemoryPool에 반납
		GMemoryPool->Deallocate(ptr, -1599);
	}
};

template<class T>
struct xvector
{
	using type = std::vector<T, STLAllocator<T>>;
};

template<class T>
struct xdeque
{
	using type = std::deque<T, STLAllocator<T>>;
};

template<class T>
struct xlist
{
	using type = std::list<T>;
};

template<class K, class T, class C = std::less<K>>
struct xmap
{
	using type = std::map<K, T, STLAllocator<std::pair<K, T>>>;
};

template<class T, class C = std::less<T>>
struct xset
{
	using type = std::set<T, C, STLAllocator<T>>;
};

template<class K, class T, class C = std::hash_compare<K, std::less<K>>>
struct xunordered_map
{
	using type = std::unordered_map<K, T, C, STLAllocator<std::pair<K, T>>>;
};

template<class T, class C = std::hash_compare<T, std::less<T>>>
struct xunordered_set
{
	using type = std::unordered_set<T, C, STLAllocator<T>>;
};

// 에러가 있다..
//template<class T, class C = std::less<std::vector<T>::value_type>>
//struct xpriority_queue
//{
//	using type = std::priority_queue<T, std::vector<T, STLAllocator<T>>, C>;
//};

using xstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, STLAllocator<wchar_t>>;