#!/bin/bash

# compte les elements
nb_elmts=$(find . -name "*.elmt" | wc -l)
nb_categ=$(find . -name "qet_directory" | wc -l)
((nb_files=$nb_elmts + $nb_categ))

echo "${nb_elmts} elements dans ${nb_categ} categories (soit ${nb_files} fichiers)"
