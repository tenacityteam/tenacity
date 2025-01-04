# Contributing to Tenacity

Thanks for showing your interest to contribute, your contribution is very
valuable to us as people like **you** help us build Tenacity. Some guidelines
have been put in place in an effort to keep the codebase clean.

**At this time, we are halting acceptance of new pull requests against the
`main` branch due to a large effort to rebase off Audacity 3.7.1. At the time
of writing, things are going smoothly so far. View PR
https://codeberg.org/tenacityteam/tenacity/pulls/527 to see the progress on the
rebase**.

**During the rebase, the rebase branch, `audacity-3.7-rebase`, will temporarily
become the default branch. Please base your PR off the `audacity-3.7-rebase`
branch and open a PR against the same branch. All other rules and procedures
regarding contributions will still apply as they did before. Thus, replace any
references to `main` below with `audacity-3.7-rebase`.**

**This notice (in bold) is temporary. Once removed, we will accept
contributions as we did before the rebase again.**

## Reporting bugs and feature requests

Our IRC channel, [`#tenacity` on Libera](https://web.libera.chat/#tenacity)
([IRC](ircs://irc.libera.chat:6697/#libera)/
[Matrix](https://matrix.to/#/#tenacity:libera.chat)), is the best place to ask
general questions about Tenacity and talk about feature requests in real time.

You can use [Codeberg Issues](https://codeberg.org/tenacityteam/tenacity/issues)
to report bugs and propose feature requests. Support questions will also be
accepted, but you must mark your support request with the 'Support' label, or
else it will be ignored and closed.

Here are some general tips if you are unsure about submitting a bug report:

* Try your best to be clear and concise.
* Be polite, as the people that will try to help you are volunteers.
* Provide as many details about the platform you are running Tenacity on as
  possible. For instance, your report should definitely contain the following:
    - What is your operating system? (Windows, macOS, Linux, Haiku, etc.)
    - Which version are you using?
    - What were you trying to do at the moment?
    - How did you install Tenacity?

## Contributing

Contributing code to Tenacity is done via Codeberg. Tenacity also requires you
to Sign-off your commits, which indicates you agree to the
[Developer Certificate of Origin](#developer-certificate-of-origin). Details
below.

### Submitting code

#### Making pull requests on Codeberg

To contribute code, first fork our
[Codeberg repository](https://codeberg.org/tenacityteam/tenacity)
and make your changes. Please use `git commit --amend` and
`git push -f` for minor changes and only if they are **your** commits.

_Note: do **not** fork our GitHub mirror for contributing. We do not accept
pull requests on GitHub anymore. All contributions **must** be made through
Codeberg._

See [git-rebase.io](https://git-rebase.io) for more details.

### Guidelines for code

Please adhere to the following guidelines when authoring code that you plan to submit to Tenacity:

1. Follow these proper code formatting guidelines:
   * **If the file uses spaces, do not change them to tabs.**

   * **Do not mix functional changes with whitespace changes**. This makes it
     difficult to review your changes because we won't what was exactly
     changed.

   * **Preserve the file's current indentation levels**. For example, if a file
     uses an indentation level of 3 spaces, don't make changes that introduce
     an indentation level of 4 spaces.

     - If creating a new file, **use an indentation level of 4 spaces. Do not
       use tabs**.

2. Do not change any variable names unless necessary.
   * You may change variable names to more understandable names, but a change
     merely consisting of variable name changes will not be accepted.

3. Follow the [commit message guidelines](#commit-messages).

#### Coding Style

Please follow the following guidelines when making your patch:

* Use PascalCase for function names and classes wherever possible and use
  camelCase for variable names.

* Prefix **protected and private** class member variables with an 'm' at the
  start using camel case. For example, if a class has a member variable called
  'data' that's protected or private, it would be named 'mData'.

* Use 3 or 4 spaces to indent your files depending on the file. **Do not use tabs**.

* Always put brackets on a new line except for variables. Example:

    ``` c++
    class Something
    {
        // stuff...
    };

    static const char* data = { /* ... */};
    ```

* Treat Tenacity libraries as system includes. Additionally, include a
  comment, `// Tenacity libraries`, that indicate the following headers are
  Tenacity libraries. This may come across as bizarre to experienced
  contributors, but it helps us maintain our code portable. Example:

  ``` c++
  // Tenacity libraries
  #include <lib-strings/Internat.h>
  ```

* For macros, use `snake_case` for naming and write the macro name in all-caps. Example:

  ``` c++
  #define SOME_MACRO 10
  #define SOME_USE_MACRO
  ```

### Guidelines for commits

#### Developer Certificate of Origin

Tenacity is an open source project licensed under the GNU General Public
license, version 2 or later (see [`LICENSE`](LICENSE.txt)).

We respect intellectual property rights, and we would like to make sure that
all contributions are properly attributed. As such, we use the simple and clear
Developer Certificate of Origin (DCO).

The DCO is a declaration attached to every contribution made by every
contributor. All the developer has to do is include a `Signed-off-by` statement,
thereby agreeing to the DCO, provided below or on
[developercertificate.org](https://developercertificate.org):

```
Developer Certificate of Origin Version 1.1

Copyright (C) 2004, 2006 The Linux Foundation and its contributors.  1
Letterman Drive Suite D4700 San Francisco, CA, 94129

Everyone is permitted to copy and distribute verbatim copies of this license
document, but changing it is not allowed.


Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I have the right
to submit it under the open source license indicated in the file; or

(b) The contribution is based upon previous work that, to the best of my
knowledge, is covered under an appropriate open source license and I have the
right under that license to submit that work with modifications, whether
created in whole or in part by me, under the same open source license (unless I
am permitted to submit under a different license), as indicated in the file; or

(c) The contribution was provided directly to me by some other person who
certified (a), (b) or (c) and I have not modified it.

(d) I understand and agree that this project and the contribution are public
and that a record of the contribution (including all personal information I
submit with it, including my sign-off) is maintained indefinitely and may be
redistributed consistent with this project or the open source license(s)
involved.
```

Each commit must include a DCO in its description, which looks like this:

```
Signed-off-by: Con Tributor <con.tributor@example.com>
```

You may type this line manually or, if using the command line, simply append
`-s` to your commit.

We ask that all contributors use their real email address, which can be replied
to.

#### Commit messages

The following rules continue to support hosting our code on multiple platforms,
such as both GitHub and Codeberg, without being unnecessarily locked in to
any single platform. Moreover, they are also necessary for complying to the GPL
license, ensuring our independence and just giving credit where it is due.

Our stance on these rules is not very strict. Worst case scenario, we may
correct the commit messages for you and inform you about what we had to correct
for future reference. However, if you would like to get seriously involved
with our project and take on responsibilities such as as reviewing and merging
patches, your compliance to the following rules will also be one of the many
factors that will be considered.

Apart from including a DCO, as mentioned earlier, the following are also very
important:

* Make concise and accurate commit messages. A commit message should be
   limited to 50 characters and its description limited to 72 characters
   per line, and the message should be able to complete this sentence:

    > This commit will...

    If you need to add any additional context, do so in the commit description.

* Avoid using full stops (e.g. `.`) or past tense in your commit messages.

   - Correct: `Add support for the Commodore 64`
   - Incorrect: `Added support for the Commodore 64.`

* The first character of the commit message should be capitalized.

   Example:

   - `CI: Buy celebratory Margherita Pizza`
   - `nyquist: Add SFX telling user to eat more veggies on startup`

* If you are using changes that were made by another person, make sure to
  properly credit them by using the `Co-authored-by: ` tag(s) in the end of
  the commit message before the `Signed-off-by:` tag(s), followed by the name
  or alias that they have used in Git in the past, as well as their e-mail.

   Example: `Co-authored-by: Jane Doe <jane.doe@example.com>`

* If your commit is complicated and involves multiple changes, use asterisks
  and explain of the changes you made in a few words.

   Example:

   ```
   Prepare Teriyaki Sauce

   * Added Soy Sauce
   * Added Cooking Sake and Sugar
   * Added 1 teaspoon of Mirin
   * Added Dashi Stock
   * Mixed ingredients together
   ```

* If you are using changes that were made by another person, the original
  changes by that said person should generally be signed off and available
  publicly in places such as another pull request on GitHub. Exceptions can
  be made, but do ***not*** sign off a commit for another person without
  their explicit permission.

* If your patch resolves an issue that was previously mentioned in the Issues
  tab on Codeberg or in our mailing list, please use the `Reference-to:` tag,
  followed by the URL where the issue in question was mentioned.

   Example:

   ```
   Reference-to: https://codeberg.org/tenacityteam/tenacity/issues/2046
   ```

* Leave an empty line between tags such as `Signed-off-by:` and the rest of
  the commit description.

   Example:

   ```
   Remove references to pumpkin pies

   I get that pumpkin pie is tasty, but this does not have anything to do
   with Tenacity whatsoever.

   Signed-off-by: Pumpkin Hater <pumpkin.hater99@example.com>
   ```

##### GitHub

* Avoid using emojis or GitHub-specific or Gitea-specific references (e.g.
  `:tada:`) to emojis in your commits. They may look just fine on GitHub, but
  they do not anywhere else.

##### Maintainers

* When merging pull requests from Codeberg, make sure to remove references to
  issues or pull requests that have a numeric format (e.g. `(#1234)` or
  `Resolves #1234`). Please use the `Reference-to:` tag instead.

  Including a hyperlink to the said issue or pull request is preferred, because
  these links will not break outside of Codeberg and will also reduce confusion
  between patches that refer to issues in the Audacity repository and patches
  that are meant to be used in Tenacity. If you use a hyperlink instead of just
  the #nnn format, Codeberg will still show the #nnn format on the website, but
  other websites and/or the command line will show the full hyperlink. It also
  makes our commits friendlier to other platforms.

###### Merging branches

* If a proposed change is running behind a certain amount of commits that affect
  the same "parts" of the project that the patch also affects, make sure to
  rebase the patch on top of the `main` branch just to be sure that the most
  recent changes do not cause the proposed patch to break.

* In order to accommodate other reviewers that live in different timezones,
  the rule of thumb is to wait for up to a day before merging a change that
  has been approved by a reviewer, or wait around 12 hours before merging a
  change that has been approved by multiple people. If possible, make sure to
  check the change for yourself if possible, especially when a reviewer
  approves a change reluctantly.

* Before merging any change, make sure that all (or, at the very least, *most
  of*) the tests have passed. If a change concerns a particular platform (e.g.
  macOS), then wait for the tests for that said platform to complete.

* If a change affects the user interface or the audio engine, you're generally
  expected to use Tenacity with the included change on your machine and
  evaluate it. Since it is very difficult to answer whether a specific change
  affecting the experience of the user is worth including or if the contributor
  should adjust their change, you may want to ask for the help of other
  contributors or community members.

* If there are multiple proposed changes that affect the same parts of the
  project, ***please wait*** for a while after initially merging a single
  proposed change just to be sure that this will not break the build. This does
  not apply to changes that do not affect the functionality of the program
  (such as changes to a Markdown file).

The most basic way of evaluating whether two separate changes affect the same
part of the project is checking whether the changes concern the same source
or header files. For example, if both changes affect `src/Theme.cpp`, then
they affect the same part of the project. However, this sort of evaluation
can get trickier, as in large applications, different source and header files
depend on each other.

Rebasing patches on top of the `main` branch and making sure that they work
as intended is the safest and fastest way to make sure that everything will be
okay.

###### Reverting

Mistakes can happen, and that's okay. After all, we're here to learn and
help others. However, reverting can impose a large amount of work on yourself
or other maintainers later down the line, as well as frustrate
contributors -- particularly those who are contributing for the first-time
or are thinking about contributing to the project.

A change that breaks Tenacity should be reverted under at least some of the
following conditions:

* There is no obvious or fast way (up to a couple of hours) to fix the mistake
  that caused Tenacity to break. **Fixing with a new commit should always be
  preferred, if possible.**
* There is a high amount of activity on the project and the change that got
  merged is killing off that said activity.
* There are multiple maintainers and contributors that are aware of this and
  agree that reverting is the best course of action.
* The community appears to heavily disagree with the change.
* Another person, regardless of whether they are a well-established developer
  or a community member, provides a new perspective that the contributors or
  maintainers were not previously aware of, which calls the integrity of the
  change into question.

When reverting a change, you should be *at least* **just as careful** as when
committing a change. Make sure to use your own judgement, communicate
transparently and coordinate together with fellow contributors. This is
especially the case with the contributors that worked on the change itself,
as throwing away their hard work without communicating or helping them have
their change included again *will* cause preventable frustration that is
absolutely preventable.
