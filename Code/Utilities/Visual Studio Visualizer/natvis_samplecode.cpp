#include <string>

#include <vector>
#include <list>
#include <map>
#include <set>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <array>


#include <Foundation/Basics.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/List.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/StaticArray.h>

#include <Core/Application/Application.h>
#include <Foundation/Configuration/Startup.h>


class Application : public ezApplication
{
  void AfterEngineInit() EZ_OVERRIDE
  {
    // string & string builder
    {
      ezString bla_ez("bla");
      ezStringBuilder bla_ez_builder("bla");
      std::string bla_std = "bla";
    }

    // dynamic array
    {
      ezDynamicArray<ezString> dynarray_ez;
      dynarray_ez.PushBack("asdf");
      dynarray_ez.PushBack("fads");

      std::vector<std::string> dynarray_std;
      dynarray_std.push_back("asdf");
      dynarray_std.push_back("fads");
    }

    // linked list
    {
      ezList<ezString> linkedlist_ez;
      linkedlist_ez.PushBack("asdf");
      linkedlist_ez.PushBack("fads");

      std::vector<std::string> linkedlist_std;
      linkedlist_std.push_back("asdf");
      linkedlist_std.push_back("fads");
    }

    // map
    {
      ezMap<ezString, ezString> map_ez;
      map_ez.Insert("asdf", "value");
      map_ez.Insert("fads", "value");

      std::map<std::string, std::string> map_std;
      map_std.insert(std::pair<std::string, std::string>("asdf", "value"));
      map_std.insert(std::pair<std::string, std::string>("fads", "value"));
    }

    // set
    {
      ezSet<ezString> set_ez;
      set_ez.Insert("asdf");
      set_ez.Insert("fads");

      std::set<std::string> set_std;
      set_std.insert("asdf");
      set_std.insert("fads");
    }

    // hashtable
    {
      ezHashTable<ezString, ezString> hashmap_ez;
      hashmap_ez.Insert("asdf", "value");
      hashmap_ez.Insert("fads", "value"); // currently some troubles here with reading the strings - even in raw view; usage works fine

      std::unordered_map<std::string, ezString> hashmap_std;
      hashmap_std.insert(std::pair<std::string, ezString>("asdf", "value"));
      hashmap_std.insert(std::pair<std::string, ezString>("fads", "value"));
    }

    // deque - doesn't work yet (chunked based design of ezDeque makes it difficult)
  /*{
      ezDeque<ezString> deque_ez;
      deque_ez.PushBack("asdf");
      deque_ez.PushBack("fads");

      std::deque<std::string> deque_std;
      deque_std.push_back("asdf");
      deque_std.push_back("fads");
    }*/

    // array
    {
      ezArrayPtr<int> pArray = EZ_DEFAULT_NEW_ARRAY(int, 100);
      EZ_DEFAULT_DELETE_ARRAY(pArray);
    }

    // static array
    {
      ezStaticArray<std::string, 2> array_ez;
      array_ez.PushBack("asdf");
      array_ez.PushBack("fads");

      std::array<std::string, 2> array_std;
      array_std[0] = "asdf";
      array_std[1] = "fads";
    }
  }

  virtual ApplicationExecution Run()
  {
    return ezApplication::Quit;
  }
};

EZ_CONSOLEAPP_ENTRY_POINT(Application)