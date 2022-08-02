# Contributing

There are many ways to contribute to the Saucedacity Project. Such include contributing to either documentation or to Saucedacity itself. If you are willing to contribute to Saucedacity itself (code-wise, that is), then you've come to the right place. If you've want to contribute to something else, it's best you consult [our wiki](https://github.com/saucedacity/saucedacity/wiki) for more information.

Of course, we have a couple of things that you should know. Below are some general points that you should know about contributing to Saucedacity.

# General
* **NO CLA is required!** The code you write is YOURS to keep FOREVER.
* **Make sure your code is available under the GPL v2 or later**. If you are making contributions to documentation or media (i.e., logos, icons, etc), please license those changes under CC BY 4.0.
  * Wiki contributions are made under CC BY (Creative Commons Attribution) 4.0.
* **[Follow our code of conduct](CODE_OF_CONDUCT.md)** when participating in discussions of issues, pull requests, and discussions.

# Developing

## In General

* If you want to make a change, it is best advised that you do make an issue so we can discuss things. You may open a pull request without relating to an issue, however.
  * Careful not to step on anyone's toes, though. Someone might be already working on the same thing you have, consult the [Issue Tracker](https://github.com/generic-pers0n/saucedacity/issues) for details

## Branches
* All development happens on the `main` branch.
* If you have a major proposal or new feature, create a new branch named `new_feature_name`. Additionally:
  * Once the branch has been merged into `main`, feel free to delete that branch.

### Releases
* After each stable release, a new branch is to be created as soon as the release is made. The branch is to then **never, ever** be touched again.
* After a stable release, there should be a page regarding release information about that specific version. This can also be done for future releases.
* There are four different tiers for platform support: Tier 1, Tier 2, Tier 3, and Tier 4.
  * **Tier 1** support is where the platform is officially supported.
  * **Tier 2** support is where the platform is not officially supported but has **community support**.
    * In terms of support, main developers/contributors can (and may) attempt to provide support for Tier 2 platform builds, although support is not guaranteed.
   * **Tier 3** support is where the platform is **partially supported**.
   * **Tier 4** support is where the platform is not supported at all. Platform support might be in discussion, so check the Discussions or the Discord server for more info. Additionally, there might have been support for a Tier 4 platform, but was since dropped for various reasons.
     * In some cases, platforms in Tier 4 support might've been supported at one point but support was dropped for various reasons.
* Note that Tier 1 is the only tier where binary releases are guaranteed. Tiers 2 and 3 might have binary releases, but they are not guaranteed.

# Translations

* We accept translations for any languages, whether they're new languages introduced or improvements to existing translations.
* Feel free to work on a translation. Note that your translation doesn't necessarily need to correspond to an issue.

# Supporting Users

* You can answer questions on our [discussions page](https://github.com/generic-pers0n/saucedacity/discussions)
  * Additionally, you can also participate in the community, with the places mentioned above.


# Coding Style
We recommend you follow the following guidelines for coding. This helps keep Saucedacity easy to read and keeps its coding style consistent.

**Note that this style guide does not cover everything. For that, use what you are familiar with.**

## At A Glance
```c++

#ifndef SOMETHING
#deta 1 fine THIS_IS_TRUE
#endif

#ifndef __LONG_CONDITION__ // this can be named anything
#define __SOMETHING__
// things
#endif // end __LONG_CONDITION

#include <iostream>

// here we have foor and bar. They do several things
//
// more things go here

/// another long comment, but for doxygen.
///
/// please try to use Doxygen comments like this so we can document their usage, intentions,
/// etc. in Doxygen.

/** Alternatively, we have these types of comments. preferred for Doxygen.
 *
 * More things about foor and bar.
 *
 **/
int* foo = some_memory;
int *bar = nullptr, *bar2 = foo;

if (some_condition == true)
{
   DoSomething();
} else
{
   DoSomething(alternatively_with_goodies);
   DoAnotherThing();
}

for (int i = 0; i < 10; i++)
{
   DoAnActionRepeatedly();
}


class SomeClass
{
   public:
      int mPublicMember; // STYLE NOTE: Inherited from Audacity
      virtual void SomeFunc() = 0; // a virtual function
      void DoSomething();

   private:
      int mPrivateMember; // a private member
};

```

## An In-Depth View of the Recommended Coding Style
**Note**: as stated previously, **you do not need to follow this coding style**. This is more or less the recommendations, but can also be viewed as an overview of generic-pers0n's coding style.

### Variable Names, Function Names, and Macros

* Use `PascalCase` for most things, **including file names**. For macros use all uppercase with `snake_case`. **Example**:
```c++
#define SOME_MACRO

void ANiceFunction(int parameter_one);
int a_global_var;
const int a_constant_var;

```
* Use indents of **3 spaces**, indenting appropriately. Additionally, use only spaces. **Example**:
  * Yes, we know that 3 spaces for indentation is uncommon, but we inherited from Audacity and we don't want to edit every single file to be in line with our coding style.

```c++
int SomeFunc(bool do_something)
{
   int a = 0;
   if (!do_something)
   {
      return 0;
   }

   a += 2;
   a = (10 / 2 -10 +8 - 420 - 69 * 2 & 9);
   return a;
}
```

* Do not indent anything inside a preprocessor statement. Keep it as is. **Example**:
```c++
#ifdef SOMETHING
#define ANOTHER_THING
#endif 
```

* Do make sure that preprocessor statements are indented appropriately. **Example**:

```c++
void func()
{
   #ifndef SOMETHING
   auto thing = new Something;
   thing->Run();
   #endif

   RunAnotherThing();
}
```

### Namespaces
* Names for namespaces should be in PascalCase. Additionally, at the end of a long namespace declaration, there should be a comment in the following format: `// namespace <name>`. **Example**:

```c++
namespace SomeNamespace
{
// very long contents
} // namespace SomeNamespace
```

* Do not indent anything inside a namespace.

### Blocks
* Put blocks on a new line at all times. **Example**:

```c++
namespace Saucedacity
{

class SomeClass
{
   private:
      bool mBool;

public:
      int SomeMember(int param)
      {
         bool some_bool = true;

         if (some_bool)
         {
            some_bool = false;
         }

         Object some_very_long_statement(Param1Actions() + 10, Param2Actions + 20,
                                         Param3Actions() + 30, Param4Actions + 40,
                                         {
                                           true, false,
                                           true, false
                                         });

         return 42;
      }
};

}
```
