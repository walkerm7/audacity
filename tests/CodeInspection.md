# Code Inspection of Top-Level [Tests](https://github.com/audacity/audacity/tree/master/tests) Directory

Most all of the code was inline with the standards and were all effecient and professional with no obvious "code smell" or inefficiencies.

However there were some uses of “naked” new and delete in some of the files. The [Coding Standard Guidlines](https://audacity.gitbook.io/dev/getting-started/coding-standards) of audacity state since 2.2.0 they have switched over to a new method that doesn't use  "naked" new or delete but instead smart pointers and containers instead.

## Small Excerpt from the Coding Standards On Used Method Instead of "new"

That is, do not manage allocated memory thus:

```C++
Obj *pObj = new Obj(x, y, z);
/* ... */
delete pObj;
```

but rather thus:

```C++
auto pObj = std::make_unique<Obj>(x, y, z);
/* ... */
```

The use of delete is now implied in the destructor of pObj, whose type is `std::unique_ptr< Obj std::default_delete< Obj > >`.

## Files Found With Naked "new" Usage

In [tests/SimpleBlockFileTest.cpp](https://github.com/audacity/audacity/blob/master/tests/SimpleBlockFileTest.cpp) uses of naked new in lines 31-33, and lines 48, 51, and 54. A function with calls to delete each object made with new starts at 60 and ends at 67. Would suggest using smart pointers instead for example instead of something like:

```C++
short *int16Data;
int16Data = new short[dataLen];
```

The guidlines suggest the use of a smart pointer with delete implied in the destructor so replacing the lines above with this:

```C++
auto int16Data = std::make_unique<short>(dataLen)
```

If necessary to delete at a specific time before desctructor or the end of the scope of the program see the [Coding Standards](https://audacity.gitbook.io/dev/getting-started/coding-standards) or ask the devs on the best approach in this matter.

Another file that had a couple uses of "naked new" was [tests/SequenceTest.cpp](https://github.com/audacity/audacity/blob/master/tests/SequenceTest.cpp) on lines 25, and 27 and then delete's corresponding to those objects at lines 34-35. The suggestion here is the same as above by replacing the "new" usage with a smart pointer or container.
