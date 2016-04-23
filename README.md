AndieGraph
==========

An Android emulator for my favorite calculator, the TI-85 (and also the TI-82, 83, 83+, and 86).

This app is backed by the excellently simple [AlmostTI][1] and available for [download][2]
on the Google Play Store. There's a [user writeup][3] on Medium.

This app was originally released in January 2011, as several apps (more suitably named) TI-82,
TI-83, TI-85, TI-86, with ROM intact; but due to a request from Texas Instruments, the ROMs
were removed and the name was changed.

Develop
-------

To build this app, you'll need the NDK. The underlying emulator, [AlmostTI][1], is written
in pure C, which runs nicely using the NDK.

Feature Requests
----------------

Most feature requests involve adding some functionality to the emulator itself. Since
the open-source version of AlmostTI has not been updated since at least 2009, it's not likely
to happen. In the meantime, its author has released a (paid) competitor to AndieGraph
on the Play Store (also called AlmostTI). I suspect there's a much newer emulation engine,
that probably also plays more nicely with Android, underlying that app. But we're stuck
with the open source version. I'll consider us lucky that he didn't remove his
open source project.

If you have a feature request that doesn't involve a feature of the emulator, feel free
to [add an issue][4]. Perhaps someone will pick it up. Or if you're an expert C developer and
want to dive into the guts of AlmostTI, that would be great!

 [1]: http://fms.komkon.org/ATI85/
 [2]: https://play.google.com/store/apps/details?id=net.supware.tipro
 [3]: https://medium.com/@dgmltn/andiegraph-update-9e9cb0b924a4#.qm4jhr738
 [4]: https://github.com/dgmltn/AndieGraph/issues/new
