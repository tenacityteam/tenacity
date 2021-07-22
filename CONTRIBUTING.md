# Contributing

There are many ways to contribute to the Saucedacity Project. Such include contributing to either documentation or to Saucedacity itself. If you are willing to contribute to Saucedacity itself (code-wise, that is), then you've come to the right place.

Of course, we have a couple of things that you should know. Below are some general points that you should know about contributing to Saucedacity.

## General
* **NO CLA is required!** The code you right is YOURS to keep FOREVER. There might be chances that you 
* **Make sure your code is available under the GPL v2 or later**. This is re

## Coding Style
**While not required**, we do recommend you follow the following guidelines for coding

### At A Glance
```c++

#ifndef SOMETHING
#define THIS_IS_TRUE
#endif

#ifndef __LONG_CONDITION__ // this can be named anything
#define __SOMETHING__
// things
#endif // end __LONG_CONDITION

#include <iostream>

// here we have foor and bar. They do several things
//
// more things go here

/** Alternatively, we have these types of comments. preferred for Doxygen.
 *
 * More things about foor and bar
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
    int a_public_member;
    virtual void SomeFunc() = 0; // a virtual function
    void DoSomething();

  private:
    int __a_private_member; // a private member
};

```

### An In-Depth View of the Recommended Coding Style
**Note**: as stated previously, **you do not need to follow this coding style**. This is more or less the recommendations, but can also be viewed as an overview of generic-pers0n's coding style.

Additionally, this is a WIP, so that justifies your own coding style

#### Variable Names, Function Names, and Macros

* Use `PascalCase` for function names, class names, and type names. Use `snake_case` for variable names. For macros use all uppercase with `snake_case`. **Example**:
```c++
#define SOME_MACRO

void ANiceFunction(int parameter_one);
int a_global_var;
const int a_constant_var;

```
* Use indents of 2 spaces, appropriately increasing as needed. **Example**:
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

* Do not indent anything inside an `#ifdef`, `#ifndef`, or similar. Keep it as is. **Example**:
```c++
#ifdef SOMETHING
#define ANOTHER_THING
#endif 
```

#### Namespaces
* Names for namespaces should be in PascalCase. Additionally, at the end of a long namespace declaration, there should be a comment in the following format: `// namespace <namespace name>`. **Example**:

```c++
namespace SomeNamespace
{
// very long contents
} // namespace SomeNamespace
```

## Developing

### In General

* If you want to make a change, you don't need to make an issue first. Go ahead and commit away! Have fun!
  * Careful not to step on anyone's toes, though. Someone might be already working on the same thing you have, consult the [Issue Tracker](https://github.com/generic-pers0n/saucedacity/issues) for details

### Branches
* All development happens on the `main` branch. Simple fixes and such go here. Do not create another branch for such simple fixes. Additionally:
  * If there are any security issues, and if you have a fix, commit them **directly** to `main` **as soon as possible**.
* If you have a major proposal or new feature, create a new branch named `new_feature_name`. Additionally:
  * Try to rebase whever possible off of `main`. This might not be necessary, but if it is, please do so.
  * Once the branch has been merged into `main`, feel free to delete that branch.

#### Releases
* After each stable release, a new branch is to be created as soon as the release is made. The branch is to then **never, ever** be touched again.
* After a stable release, there should be a page regarding release information about that specific version. This can also be done for future releases.
* There are four different tiers for platform support: Tier 1, Tier 2, Tier 3, and Tier 4.
  * **Tier 1** support is where the platform is officially supported.
  * **Tier 2** support is where the platform is not officially supported but has **community support**.
    * In terms of support, main developers/contributors can attempt to answer support requests regarding Tier 2 platform builds, although support is not guaranteed.
   * **Tier 3** support is where the platform is **partially supported**.
   * **Tier 4** support is where the platform is not supported at all. Platform support might be discussed, so check the Discussions or the Discord server for more info.
     * In some cases, platforms in Tier 4 support might've been supported at one point but support was dropped for various reasons.


## Translation

* We accept translations for any languages, whether they're new languages introduced or improvements to existing translations.
* Feel free to work on a translation. Note that your translation doesn't necessarily need to correspond to an issue.

## Supporting Users

* You can answer questions in the [discussions](https://github.com/generic-pers0n/saucedacity/discussions) and on our [Discord server](https://discord.gg/UXbWPpB422).
  * Additionally, you can also participate in the community, with the aforementioned places also.
