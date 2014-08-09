Mishira
=======

Mishira is a personal audiovisual production tool used for editing and broadcasting live video over the internet that has been specifically designed for people who play video games. Mishira allows you to capture, record and broadcast your PC's desktop and speakers and mix them together with your own images, text, webcams, microphones, visual effects and transitions to form a single professional video broadcast. Mishira is completely free and open source software.

This README is written to help existing and future contributors of the Mishira project. For a user-friendly overview of the project as well as for user support and other services please visit the official Mishira website at [www.mishira.com](http://www.mishira.com).

The Mishira user community can regularly be found in the [#mishira](irc://irc.freenode.org/mishira) IRC channel on the [Freenode](http://www.freenode.org/) network. Developers usually also hang out in [#mishira-devel](irc://irc.freenode.org/mishira-devel) from time to time. You can access the user channel via a web interface by clicking [here](http://webchat.freenode.net/?channels=mishira).

Mission statement
=================

> Our mission is to create a streamlined and polished user experience for recording and broadcasting video over the internet using entirely free and open source software.

License
=======

Unless otherwise specified all parts of the Mishira project are licensed under the standard GNU General Public License v2 or later versions. A copy of this license is available in the `LICENSE.GPL.txt` file.

The license that end users agree too when installing and using the binary is known as the Mishira Software License Agreement and contains provisions to help protect contributors from legal action while notifying the end users that the software itself is licensed under the GPL. The GPL cannot be used by itself as an EULA as it does not cover any action other than copying, distribution and modification.

Any questions regarding the licensing of Mishira can be sent to the authors via their provided contact details.

Version history
===============

A consolidated version history of the Mishira application can be found in the `CHANGELOG.md` file.

Dependency map
==============

A basic overview diagram of Mishira's dependencies can be found in `dependencies.png`. If you are viewing this file on GitHub it will be embedded below.

![Mishira dependency map](dependencies.png?raw=true)

Building
========

Instructions for building Mishira from source can be found in the `BUILDING.md` file. If you would just like to use Mishira you can find download links to the binaries on the official Mishira website at [www.mishira.com](http://www.mishira.com).

Mishira vs OBS
==============

[Open Broadcaster Software](http://www.obsproject.com/) (OBS) is a free and open source video recording and live streaming software project that overlaps Mishira's goals in many places. Although the OBS project started before Mishira it was required that Mishira be developed completely independently of OBS as it was intended to be a commercial closed source product. During the time that Mishira was in development behind closed doors the OBS team begun a rewrite initiative that was aimed to fix many of the issues that the original code had and to allow multiple user interfaces to share the same underlying broadcaster code. Now that Mishira is also an open source project there are some conflicts between the two projects.

As Mishira's goal is only to provide a "streamlined and polished user experience" then if OBS's underlying library can be reused by Mishira without causing any regressions to existing features then it is beneficial for both projects to make them converge into a single strong initiative. Although it may take a very long time to replace Mishira's core with the OBS library it is definitely something that should be worked towards, at least in the long term. If you would like to help make this happen please get in contact with us!

Contributing
============

Want to help out on Mishira? We don't stop you! New contributors to the project are always welcome even if it's only a small bug fix or even if it's just helping spread the word of the project to others. You don't even need to ask; just do it!

Source code
-----------

The Mishira source code is [hosted on GitHub](https://www.github.com/mishira). As the Mishira project contains multiple Git repositories in order to build a usable binary from the source code you will need to clone each repository and follow the building instructions that can be found in the `BUILDING.md` file of each repository.

Issue tracker
-------------

The [GitHub issue tracker](https://github.com/mishira/mishira/issues) in the main Mishira repository is the preferred channel for bug reports, feature requests and submitting pull requests.

Bug reports
-----------

A bug is a *demonstrable problem* in the current version of the application. Bugs are a major cause of user frustration and should always be prioritised over adding new features. Good bug reports are extremely helpful so if you find a bug please let us know via our issue tracker!

Before submitting a bug report please first spend the time searching the issue tracker to see if it has already been reported by somebody else. If you find an existing report feel free to add additional information to it if you believe it would be helpful for developers.

Feature requests
----------------

Feature requests are always welcome but please take a moment to find out if your idea fits within the scope and goals of the project before submitting it to our issue tracker. Mishira is not meant to be a jack-of-all-trades video editor so having too many features can result in buggy unmaintained code that will cause more user frustrations than what it's worth. It is up to *you* to make a strong case to convince the project's developers of the merits of your requested feature. Please provide as much detail and context as possible.

If you are uncertain as to whether or not your feature request is within the scope of the project feel free to contact the developers via the IRC channel (Preferred) or E-mail.

Pull requests
-------------

Good pull requests containing patches, improvements and new features are a fantastic way to help the project. Before submitting a pull request please make sure that any source code follows the same coding style as what is already used in the repository and that the Git history does not contain unrelated commits.

**Before starting any significant work** please make sure that you contact the existing developers to ensure that your pull request is likely to be accepted and merged into the official repository. You don't want to spend a lot of time working on something that is unlikely to be accepted due to being outside the scope of the project or if its already been worked on by somebody else.

If working on a part of the project that contains unit tests please also write additional tests for your code before you submit the pull request. While unit tests can take time to write they are useful for ensuring that any written code continues to operating correctly into the future even after you stop contributing to the project. Many parts of Mishira have very strict performance requirements and even the smallest change elsewhere in the project could result in subtle performance bugs that are only visible to users with certain hardware configurations.

Acknowledgements
================

Mishira was originally developed solely by [Lucas Murray](https://www.github.com/lmurray) and was planned to be released as a commercial product. Unfortunately (Or fortunately depending on your perspective) as market and life situations change over the first year of development the commercial aspect of the project was dropped and it has since been converted to be a completely free and open source project for everybody to benefit from.

Mishira could not be what it is today without the help of these people:

- [Lucas Murray](https://www.github.com/lmurray) (Lead developer and project maintainer),
- [Tim Oliver](http://www.timoliver.com.au/) (Website design),
- Everybody else that helps contribute in some small way,
- The developers of all third-party libraries that Mishira uses, and
- *You*, for being an awesome person.
