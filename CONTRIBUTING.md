# How to contribute
 
I'm really glad you're reading this,
because we need volunteer developers to help
this project come to fruition.


Here are some important resources:

* [Qet code style](https://qelectrotech.org/wiki_new/doc/qt_creator#on_ajoute_le_style_de_code_qet)
* [git Documentation](https://git-scm.com/doc)

## Testing

## Submitting changes

Always write a clear log message for your commits.
One-line messages are fine for small changes,
but bigger changes should look like this:

    $ git commit -m "A brief summary of the commit
    > 
    > A paragraph describing what changed and its impact."

* It is always appropriate to keep the commits small.
* For major changes it is recommended to use branches.

### Interactive Staging
https://git-scm.com/book/en/v2/Git-Tools-Interactive-Staging

issue: you have modified a class but you want to write it in 2 commits

 ´git add -p´ or ´git add -i´


    /qet> git add -i


               staged     unstaged path
      1:    unchanged        +1/-1 sources/diagram.cpp

    *** Commands ***
      1: status       2: update       3: revert       4: add untracked
      5: patch        6: diff         7: quit         8: help


    What now> 5


               staged     unstaged path
      1:    unchanged        +1/-1 sources/diagram.cpp
    Patch update>> 1
               staged     unstaged path
    * 1:    unchanged        +1/-1 sources/diagram.cpp
    Patch update>>
    diff --git a/sources/diagram.cpp b/sources/diagram.cpp
    index bffca653f..9bd2280f7 100644
    --- a/sources/diagram.cpp
    +++ b/sources/diagram.cpp
    @@ -103,9 +103,9 @@ Diagram::Diagram(QETProject *project) :
        connect(&border_and_titleblock,
                &BorderTitleBlock::titleBlockFolioChanged,
                this, &Diagram::updateLabels);
    -       connect(this, &Diagram::diagramActivated,
    +       foo(do_a);
    -       adjust(diagramActivated);
    +       bar(do_c);
        adjustSceneRect();
     }


    (1/1) Stage this hunk [y,n,q,a,d,s,e,?]? s


    Split into 2 hunks.
    @@ -103,5 +103,5 @@
        connect(&border_and_titleblock,
                &BorderTitleBlock::titleBlockFolioChanged,
                this, &Diagram::updateLabels);
    -       connect(this, &Diagram::diagramActivated,
    +       foo(do_a);


    (1/2) Stage this hunk [y,n,q,a,d,j,J,g,/,e,?]? y


    @@ -107,5 +107,5 @@
                this, &Diagram::loadElmtFolioSeq);
    -       adjust(diagramActivated);
    +       bar(do_c);
        adjustSceneRect();
     }


    (2/2) Stage this hunk [y,n,q,a,d,K,g,/,e,?]? n


    *** Commands ***
      1: status       2: update       3: revert       4: add untracked
      5: patch        6: diff         7: quit         8: help
    What now>What now>7
    Bye.

    git commit -m "Mod Signal Slot to funsion"

## Coding conventions

Start reading our code and you'll get the hang of it.
We optimize for readability:

  * We use tabs to indent, and interpret tabs as taking up to 8 spaces.
    see https://qelectrotech.org/wiki_new/doc/qt_creator#on_ajoute_le_style_de_code_qet
  * We try to keep to at most 80 characters per line.
  * Try to make your code understandable.
    You may put comments in, but comments invariably tend to stale out when
    the code they were describing changes.
    Often splitting a function into two
    makes the intention of the code much clearer.
    
Thanks,
QElectroTech
