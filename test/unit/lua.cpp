#include <configure/lua/State.hpp>
#include <configure/lua/Type.hpp>

#include <boost/filesystem.hpp>
#include <boost/test/output_test_stream.hpp>

namespace lua = configure::lua;
namespace fs = boost::filesystem;

namespace {

	struct stream_capture
	{
	private:
		boost::test_tools::output_test_stream _test_stream;

	public:
		struct Lock
		{
			std::streambuf* _old;
			Lock(std::streambuf* buf)
				: _old(std::cout.rdbuf(buf))
			{}
			~Lock()
			{ std::cout.rdbuf(_old); }
		};

	public:
		std::unique_ptr<Lock> lock_guard()
		{ return std::unique_ptr<Lock>(new Lock(_test_stream.rdbuf())); }

		boost::test_tools::output_test_stream& out() { return _test_stream; }
	};

	fs::path const& test_dir()
	{
		static fs::path res = fs::absolute(fs::path(__FILE__).parent_path());
		return res;
	}

	fs::path lua_test(std::string const& name)
	{
		return test_dir() / "lua" / (name + ".lua");
	}

}

BOOST_AUTO_TEST_CASE(load_file)
{
	{
		lua::State state(false);
		BOOST_CHECK_THROW(state.load(lua_test("hello")), std::exception);
	}
	{
		stream_capture capture;
		lua::State state(true);
		{
			auto guard = capture.lock_guard();
			state.load(lua_test("hello"));
		}
		BOOST_CHECK(capture.out().is_equal("Hello, World !\n"));
	}
	{
		stream_capture capture;
		lua::State state(true);
		{
			auto guard = capture.lock_guard();
			state.load(std::string("print('Hello,', 'World', '!')\n"));
		}
		BOOST_CHECK(capture.out().is_equal("Hello, World !\n"));
	}
}

BOOST_AUTO_TEST_CASE(load_invalid_file)
{
	lua::State state;
	BOOST_CHECK_THROW(state.load(fs::path("THIS-FILE-DOES-NOT-EXISTS")), std::exception);
}

BOOST_AUTO_TEST_CASE(raw_lua_call)
{
	lua::State state;
	int top = state.gettop();
	state.load(lua_test("function"));
	state.getglobal("add");
	state.push(12);
	state.push(13);
	BOOST_CHECK_EQUAL(state.gettop(), top + 3);
	state.call(2, 1);
	BOOST_CHECK_EQUAL(state.gettop(), top + 1);
	BOOST_CHECK_EQUAL(state.to<int>(), 25);
	state.pop();
	BOOST_CHECK_EQUAL(state.gettop(), top);
}

BOOST_AUTO_TEST_CASE(call_std_function)
{
	lua::State state;
	int top = state.gettop();
	bool called = false;
	state.push_callable(std::function<void()>([&called] { called = true;  }));
	BOOST_CHECK_EQUAL(called, false);
	state.call(0);
	BOOST_CHECK_EQUAL(called, true);
	BOOST_CHECK_EQUAL(state.gettop(), top);
}

BOOST_AUTO_TEST_CASE(globals)
{
	lua::State state;
	int top = state.gettop();
	state.global("a", 12);
	BOOST_CHECK_EQUAL(state.gettop(), top);
	BOOST_CHECK_EQUAL(state.global<int>("a"), 12);
	BOOST_CHECK_EQUAL(state.gettop(), top);
}

BOOST_AUTO_TEST_CASE(dtor_called)
{
	bool called = false;

	struct A {
		bool& _value;
		A(bool& value) : _value(value) {}
		~A() { _value = true; }
	};
	{
		lua::State s;
		s.construct<A>(called);
		BOOST_CHECK_EQUAL(called, false);
	}
	BOOST_CHECK_EQUAL(called, true);
}

BOOST_AUTO_TEST_CASE(bind_void)
{
	lua::State s;
	bool called = false;
	s.push_callable(std::function<void()>([&called] { called = true; }));
	BOOST_CHECK_EQUAL(called, false);
	s.call(0, 0);
	BOOST_CHECK_EQUAL(called, true);
}

BOOST_AUTO_TEST_CASE(bind_int)
{
	lua::State s;
	s.push_callable(std::function<int()>([] { return 42; }));
	s.call(0, 1);
	BOOST_CHECK_EQUAL(s.to<int>(), 42);
}

BOOST_AUTO_TEST_CASE(bind_int_int)
{
	lua::State s;
	s.push_callable(std::function<int(int)>([] (int i){ return i+1; }));
	s.push(12);
	s.call(1, 1);
	BOOST_CHECK_EQUAL(s.to<int>(), 13);
}

BOOST_AUTO_TEST_CASE(bind_int_int_int)
{
	lua::State s;
	s.push_callable(std::function<int(int, int)>([] (int i, int j){ return i+j; }));
	s.push(12);
	s.push(12);
	s.call(2, 1);
	BOOST_CHECK_EQUAL(s.to<int>(), 24);
}

BOOST_AUTO_TEST_CASE(bind_string_string)
{
	lua::State s;
	s.push_callable(std::function<std::string(std::string const&)>([] (std::string const& s){ return s + "-LOL"; }));
	s.push("COUCOU");
	s.call(1, 1);
	BOOST_CHECK_EQUAL(s.to<std::string>(), "COUCOU-LOL");
}

BOOST_AUTO_TEST_CASE(bind_ref_builtins)
{
	lua::State s;
	s.push_callable(std::function<int const&()>());
	s.push_callable(std::function<int&()>());
	s.push_callable(std::function<std::string const&()>());
	s.push_callable(std::function<std::string&()>());
	BOOST_CHECK("Just compile time check");
}

struct TestType
{
	void test_void() { std::cout << "Hello !\n"; }
	int test_int() { return 42; }
};

BOOST_AUTO_TEST_CASE(simple_type)
{
	lua::State s;
	lua::Type<TestType>(s)
		.def("test_void", &TestType::test_void)
		.def("test_int", &TestType::test_int)
	;
	s.construct<TestType>();
	s.setglobal("value");

	s.load("assert(value:test_int() == 42)");
	BOOST_CHECK_THROW(s.load("assert(value:test_int() == 43)"), std::exception);
}

BOOST_AUTO_TEST_CASE(ref_type)
{
	lua::State s;

	lua::Type<TestType, std::reference_wrapper<TestType>>(s)
		.def("test_void", &TestType::test_void)
		.def("test_int", &TestType::test_int)
	;
	TestType the_one;
	s.construct<std::reference_wrapper<TestType>>(the_one);
	s.setglobal("value");

	s.load("assert(value:test_int() == 42)");
	BOOST_CHECK_THROW(s.load("assert(value:test_int() == 43)"), std::exception);
}

