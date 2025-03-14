#!/bin/bash

# Paramètres de la simulation
LONGUEUR=1.0          # Taille du terrain
DISCRETISATION=200    # Nombre de cellules par direction
VENT_X=1.0            # Composante X du vent
VENT_Y=0.0            # Composante Y du vent
FOYER_X=0.2           # Position X du foyer initial
FOYER_Y=0.5           # Position Y du foyer initial

echo "Paramètres de la simulation :"
echo "  Longueur du terrain : $LONGUEUR"
echo "  Discrétisation : $DISCRETISATION"
echo "  Vent : ($VENT_X, $VENT_Y)"
echo "  Position initiale du foyer : ($FOYER_X, $FOYER_Y)"
echo

./simulation.exe -l $LONGUEUR -d $DISCRETISATION -w $VENT_X $VENT_Y -s $FOYER_X $FOYER_Y 