#include "State.hpp"
#include "traceback.hpp"
#define BOOST_POOL_INSTRUMENT
#include <boost/algorithm/string.hpp>
#include <boost/pool/pool.hpp>
#include <boost/assert.hpp>
#include <boost/scope_exit.hpp>

#include <iostream>
#include <map>

namespace configure { namespace lua {

	struct State::LuaAllocator
	{
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;
		//static size_t count;

		static char * malloc(const size_type bytes)
		{
			//count += 1;
			return reinterpret_cast<char *>(std::malloc(bytes));
		}

		static void free(char * const block)
		{
			std::free(block);
		}

	};

	//size_t State::LuaAllocator::count = 0;

	namespace {

		static void *lua_naive_allocator(void*,
		                                 void* ptr,
		                                 size_t original_size,
		                                 size_t new_size)
		{
			if (new_size == 0)
			{
				std::free(ptr);
				ptr = nullptr;
			}
			else if (original_size == 0 || ptr == nullptr)
			{
				ptr = std::malloc(new_size);
			}
			else if (new_size > original_size)
			{
				ptr = std::realloc(ptr, new_size);
			}
			return ptr;
		}

		static void *lua_allocator(void *payload,
		                          void *ptr,
		                          size_t original_size,
		                          size_t new_size)
		{
			// The type of the memory-allocation function used by Lua states.
			// The allocator function must provide a functionality similar to
			// realloc, but not exactly the same.
			// Its arguments are:
			//   - payload: an opaque pointer passed to lua_newstate;
			//   - ptr: a pointer to the block being allocated/reallocated/freed;
			//   - original_size: the original size of the block;
			//   - new_size: the new size of the block.
			//
			// ptr is NULL if and only if original_size is zero.
			//
			// When new_size is zero, the allocator must return NULL;
			//
			// if original_size is not zero, it should free the block pointed
			// to by ptr.
			//
			// When new_size is not zero, the allocator returns NULL if and
			// only if it cannot fill the request.
			//
			// When new_size is not zero and original_size is zero, the
			// allocator should behave like malloc.
			//
			// When new_size and original_size are not zero, the allocator behaves
			// like realloc.
			//
			// Lua assumes that the allocator never fails when original_size >= new_size.

			auto& pool = *static_cast<State::Pool*>(payload);


			//struct Stats {
			//	std::map<size_t, size_t> raw_mallocs;
			//	std::map<size_t, size_t> pool_mallocs;
			//	~Stats() {
			//		size_t sum = 0;

			//		for (auto& p: raw_mallocs)
			//			sum += p.second;

			//		std::cout << "Raw mallocs: " << sum << "\n";
			//		for (auto& p: raw_mallocs)
			//			std::cout << p.first << '\t' << p.second << " times\n";

			//		sum = 0;
			//		for (auto& p: pool_mallocs)
			//			sum += p.second;
			//		std::cout << "Pool mallocs: " << sum << "\n";
			//		for (auto& p: pool_mallocs)
			//			std::cout << p.first << '\t' << p.second << " times\n";

			//		std::cout << "Pool chunks skipped realloc'd: " << skipped_realloc << std::endl;
			//		std::cout << "Pool mallocs: " << State::LuaAllocator::count << std::endl;
			//		//std::cout << "Raw mallocs: " << raw_mallocs << std::endl;
			//		//std::cout << "Pool mallocs: " << pool_mallocs << std::endl;
			//	}
			//	size_t skipped_realloc = 0;
			//};
			//static Stats stats;


			//struct Tracker
			//{
			//	std::map<void*, size_t> pointers;
			//	~Tracker()
			//	{
			//		for (auto& p: pointers)
			//			std::cout << "Leak of " << p.first << " of " << p.second << " bytes\n";
			//	}

			//	void remove(void* ptr, size_t size)
			//	{
			//		assert(pointers.at(ptr) == size);
			//		pointers.erase(ptr);
			//	}
			//	void add(void* ptr, size_t size)
			//	{
			//		assert(pointers.count(ptr) == 0);
			//		pointers[ptr] = size;
			//	}
			//	void move(void* old, size_t osize, void* new_, size_t nsize)
			//	{
			//		this->remove(old, osize);
			//		this->add(new_, nsize);
			//	}
			//};
			//static Tracker tracker;

			size_t chunk_size = pool.get_requested_size();
			if (new_size == 0)
			{
				// pool does not check for null pointer
				if (ptr != nullptr)
				{
					if (original_size > chunk_size)
						std::free(ptr);
					else
						pool.free(ptr);
					//tracker.remove(ptr, original_size);
					ptr = nullptr;
				}
			}
			else if (original_size == 0 || ptr == nullptr)
			{
				assert(ptr == nullptr);
				if (new_size > chunk_size)
				{
					ptr = std::malloc(new_size);
					//stats.raw_mallocs[new_size] += 1;
				}
				else
				{
					ptr = pool.malloc();
					//stats.pool_mallocs[new_size] += 1;
				}
				//tracker.add(ptr, new_size);
			}
			else if (new_size > original_size)
			{
				if (original_size > chunk_size)
				{
					// Memory wasn't handled by the memory pool
					void* new_ptr = std::realloc(ptr, new_size);
					if (new_ptr == nullptr)
					{
						std::free(ptr);
						return nullptr;
					}
					//tracker.move(ptr, original_size, new_ptr, new_size);
					ptr = new_ptr;
					//stats.raw_mallocs[new_size] += 1;
				}
				else if (new_size > chunk_size)
				{
					// original chunk was handled by the bool
					// the new one will be handled by malloc
					void* new_ptr = std::malloc(new_size);
					std::memcpy(new_ptr, ptr, original_size);
					pool.free(ptr);
					//tracker.move(ptr, original_size, new_ptr, new_size);
					ptr = new_ptr;
					//stats.raw_mallocs[new_size] += 1;
				}
				else
				{
					//  The new size fits in a chunk
					// tracker.move(ptr, original_size, ptr, new_size);
				}
			}
			else
			{
				assert(new_size <= original_size && ptr != nullptr);
				if (original_size > chunk_size && new_size <= chunk_size)
				{
					// We must move that chunk to the pool
					void* new_ptr = pool.malloc();
					std::memcpy(new_ptr, ptr, new_size);
					std::free(ptr);
					//tracker.move(ptr, original_size, new_ptr, new_size);
					ptr = new_ptr;
				}
				//else
				//	tracker.move(ptr, original_size, ptr, new_size);
			}
			return ptr;
		}

		static int lua_panic(lua_State* state)
		{
			CONFIGURE_THROW(error::LuaError(lua_tostring(state, -1)));
		}

	}

	State::State(bool with_libs)
		: _state(nullptr)
		, _owner(true)
		, _pool(new Pool(64, 1024))
	{
		if (getenv("CONFIGURE_USE_NAIVE_ALLOCATOR") != nullptr)
			_state = lua_newstate(&lua_naive_allocator, _pool.get());
		else
			_state = lua_newstate(&lua_allocator, _pool.get());
		if (_state == nullptr)
			throw std::bad_alloc();
		lua_atpanic(_state, &lua_panic);
		if (with_libs)
		{
			luaL_openlibs(_state);
			lua_register(_state, "print", &_print_override);
			_register_extensions();
		}
	}

	State::~State()
	{
		if (_owner)
		{
			luaL_unref(_state, LUA_REGISTRYINDEX, _error_handler_ref);
			lua_close(_state);
		}
	}

	void State::check_status(lua_State* L, int status)
	{
		static std::map<int, std::string> error_strings{
			{LUA_ERRRUN, "Runtime error"},
			{LUA_ERRSYNTAX, "Syntax error"},
			{LUA_ERRMEM, "Memory error"},
			{LUA_ERRGCMM, "GC error"},
			{LUA_ERRERR, "Error handling error"},
		};
		switch (status)
		{
		case LUA_OK:
		case LUA_YIELD:
			return;
		case LUA_ERRRUN:
		case LUA_ERRSYNTAX:
		case LUA_ERRMEM:
		case LUA_ERRGCMM:
		case LUA_ERRERR:
			break;
		default:
			throw std::runtime_error("Unknown lua status: " + std::to_string(status));
		}
		if (auto e = Converter<std::exception_ptr>::extract_ptr(L, -1))
		{
			std::exception_ptr cpy = *e;
			lua_remove(L, -1);
			std::rethrow_exception(cpy);
		}
		std::string msg = error_strings[status] + ": ";
		if (char const* str = lua_tostring(L, -1))
		{
			msg.append(str);
			lua_remove(L, -1);
		}
		else
			msg.append("Unknown error");
		CONFIGURE_THROW(
			error::LuaError(msg) << error::lua_traceback(traceback(L))
		);
	}

	namespace {

		int string_starts_with(lua_State* state)
		{
			char const* s1 = lua_tostring(state, 1);
			char const* s2 = lua_tostring(state, 2);
			if (s1 == nullptr || s2 == nullptr)
			{
				lua_pushstring(state, "string:starts_with() expect one argument");
				lua_error(state);
			}
			if (boost::algorithm::starts_with(s1, s2))
				lua_pushboolean(state, 1);
			else
				lua_pushboolean(state, 0);
			return 1;
		}

		int string_ends_with(lua_State* state)
		{
			char const* s1 = lua_tostring(state, 1);
			char const* s2 = lua_tostring(state, 2);
			if (s1 == nullptr || s2 == nullptr)
			{
				lua_pushstring(state, "string:starts_with() expect one argument");
				lua_error(state);
			}
			if (boost::algorithm::ends_with(s1, s2))
				lua_pushboolean(state, 1);
			else
				lua_pushboolean(state, 0);
			return 1;
		}

		enum class StripAlgo
		{
			left,
			right,
			both,
		};

		template <StripAlgo algo>
		int string_strip(lua_State* state)
		{
			char const* s = lua_tostring(state, 1);
			char const* to_remove = lua_tostring(state, 2);
			if (s == nullptr)
			{
				lua_pushstring(state, "string:strip() expect at least one argument");
				lua_error(state);
			}
			if (to_remove == nullptr)
				to_remove = "\n\t\r ";

			auto pred = boost::algorithm::is_any_of(to_remove);

			std::string res = s;

			if (algo == StripAlgo::left)
				boost::algorithm::trim_left_if(res, pred);
			if (algo == StripAlgo::right)
				boost::algorithm::trim_right_if(res, pred);
			if (algo == StripAlgo::both)
				boost::algorithm::trim_if(res, pred);

			lua_pushstring(state, res.c_str());
			return 1;
		}

		int string_split(lua_State* state)
		{
			char const* s = lua_tostring(state, 1);
			char const* tokens = lua_tostring(state, 2);
			if (s == nullptr)
			{
				lua_pushstring(state, "string:split() expect at least one argument");
				lua_error(state);
			}
			if (tokens == nullptr)
				tokens = "\n\t\r ";
			auto it = boost::algorithm::make_split_iterator(
				s,
				boost::algorithm::token_finder(
					boost::algorithm::is_any_of(tokens),
					boost::algorithm::token_compress_on
				)
			);
			lua_createtable(state, 0, 0);
			int idx = 1;
			while (!it.eof())
			{
				lua_pushlstring(state, it->begin(), it->size());
				lua_rawseti(state, -2, idx);
				++it;
				++idx;
			}
			return 1;
		}

		int table_append(lua_State* state)
		{
			// table : value
			int len = lua_rawlen(state, 1);
			lua_rawseti(state, 1, len + 1);
			// table
			return 1;
		}

		int table_extend(lua_State* state)
		{
			if ((lua_gettop(state) != 2) || !lua_istable(state, 1) || !lua_istable(state, 2))
			{
				lua_pushstring(state, "table.extend(): Two arguments of type table expected");
				lua_error(state);
			}
			// table1 : table2
			int len1 = lua_rawlen(state, 1);
			int len2 = lua_rawlen(state, 2);
			for (int i = 1; i <= len2; ++i)
			{
				lua_rawgeti(state, 2, i);
				// t1 : t2 : t2[i]
				lua_rawseti(state, 1, len1 + i);

			}
			lua_remove(state, -1);
			return 1;
		}

		int table_update(lua_State* state)
		{
			if ((lua_gettop(state) != 2) || !lua_istable(state, 1) || !lua_istable(state, 2))
			{
				lua_pushstring(state, "table.update(): Two arguments of type table expected");
				lua_error(state);
			}
			lua_pushnil(state);
			// -3 : -2 : -1
			// t1 : t2 : nil
			while (lua_next(state, -2)) // iterate through t2
			{
				// -4 : -3 : -2  : -1
				// t1 : t2 : key : value

				lua_pushvalue(state, -2);
				// -5 : -4 : -3  : -2    : -1
				// t1 : t2 : key : value : key

				lua_insert(state, -2);
				// -5 : -4 : -3  : -2  : -1
				// t1 : t2 : key : key : value

				// t1[key] = value
				lua_settable(state, -5);
				// -3 : -2  : -1
				// t1 : t2 : key
			}
			// t1 : t2
			lua_pop(state, 1);
			return 1;
		}

		int table_tostring(lua_State* state)
		{
			if (!lua_istable(state, -1))
			{
				luaL_error(state,
				           "table.tostring(): One argument of type table expected");
			}
			std::vector<std::string> strings;
			lua_pushnil(state);
			assert(lua_istable(state, -2));
			while (lua_next(state, -2))
			{
				if (char const* s = luaL_tolstring(state, -1, nullptr))
					strings.push_back(s);
				else
					strings.push_back("(nil)");
				lua_pop(state, 2);
				assert(lua_istable(state, -2));
			}
			lua_pushstring(state, ("{" + boost::join(strings, ", ") + "}").c_str());
			return 1;
		}

		int error_handler(lua_State* state)
		{
			if (char const* str = lua_tostring(state, -1))
			{
				Converter<std::exception_ptr>::push(
					state,
					std::make_exception_ptr(
						error::LuaError(str)
							<< error::lua_traceback(traceback(state))
					)
				);
			}
			else if (auto ptr = Converter<std::exception_ptr>::extract_ptr(state, -1))
			{
				try { std::rethrow_exception(*ptr); }
				catch (error::LuaError&) {
					/* Right type already */
				}
				catch (...) {
					Converter<std::exception_ptr>::push(
						state,
						std::make_exception_ptr(
							error::LuaError()
								<< error::lua_traceback(traceback(state))
								<< error::nested(*ptr)
						)
					);
				}
			}
			else
			{
				Converter<std::exception_ptr>::push(
					state,
					std::make_exception_ptr(
						error::LuaError()
							<< error::lua_traceback(traceback(state))
					)
				);
			}
			return 1;
		}

	}


	void State::_register_extensions()
	{
#define SET_METHOD(method, ptr) \
		lua_pushstring(_state, method); \
		lua_pushcfunction(_state, ptr); \
		lua_rawset(_state, -3); \
/**/
		lua_getglobal(_state, "string");
		SET_METHOD("starts_with", &string_starts_with);
		SET_METHOD("ends_with", &string_ends_with);
		SET_METHOD("strip", &string_strip<StripAlgo::both>);
		SET_METHOD("rstrip", &string_strip<StripAlgo::right>);
		SET_METHOD("lstrip", &string_strip<StripAlgo::left>);
		SET_METHOD("split", &string_split);

		lua_getglobal(_state, "table");
		SET_METHOD("append", &table_append);
		SET_METHOD("extend", &table_extend);
		SET_METHOD("update", &table_update);
		SET_METHOD("tostring", &table_tostring);

		lua_settop(_state, 0);
		lua_pushcfunction(_state, &error_handler);
		_error_handler_ref = luaL_ref(_state, LUA_REGISTRYINDEX); // add ref to avoid gc
#undef SET_METHOD

	}

	void State::forbid_globals()
	{
		std::string code =
			"local mt = {}\n"
			"setmetatable(_G, mt)\n"
			"mt.__newindex = function(g, k, v)\n"
			"	error('Trying to create a global variable \"' .. tostring(k) ..'\"')\n"
			"end\n"
		;
		this->load(code);
	}

	void State::load(boost::filesystem::path const& p, int ret)
	{
		check_status(_state, luaL_loadfile(_state, p.string().c_str()));
		this->call(0, ret);
	}

	void State::load(std::string const& buffer, int ret)
	{ this->load(buffer.c_str(), ret); }

	void State::load(char const* buffer, int ret)
	{
		check_status(_state, luaL_loadstring(_state, buffer));
		this->call(0, ret);
	}

	void State::call(int nargs, int nresults)
	{
		if (nresults == -1)
			nresults = LUA_MULTRET;

		// We insert the error handler before the function on the stack.
		// When the function stack is cleaned up, the handler is still there.
		// This is not efficient, but we do not call lua function very often.
		int error_handler = _error_handler(-1 - nargs - 1);
		BOOST_SCOPE_EXIT_ALL(&){ lua_remove(_state, error_handler); };

		int res = lua_pcall(
			_state,
			nargs,
			nresults,
			error_handler
		);
		this->check_status(_state, res);
	}

	int State::_print_override(lua_State* L)
	{
		int count = lua_gettop(L);
		if (count >= 1)
			std::cout << luaL_tolstring(L, 1, nullptr);
		for (int i = 2; i <= count; ++i)
			std::cout << ' ' << luaL_tolstring(L, i, nullptr);
		std::cout << std::endl;
		return LUA_OK;
	}

	int State::_error_handler(int insert_at)
	{
		lua_rawgeti(_state, LUA_REGISTRYINDEX, _error_handler_ref);
		int idx = lua_absindex(_state, insert_at);
		lua_insert(_state, idx);
		return idx;
	}
}}
