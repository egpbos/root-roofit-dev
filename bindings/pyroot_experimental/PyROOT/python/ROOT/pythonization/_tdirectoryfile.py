# Author: Danilo Piparo, Stefan Wunsch CERN  08/2018

################################################################################
# Copyright (C) 1995-2018, Rene Brun and Fons Rademakers.                      #
# All rights reserved.                                                         #
#                                                                              #
# For the licensing terms see $ROOTSYS/LICENSE.                                #
# For the list of contributors see $ROOTSYS/README/CREDITS.                    #
################################################################################

from ROOT import pythonization

# TDirectoryFile inherits from TDirectory the pythonized attr syntax (__getattr__)
# and WriteObject method.
# We just need to re-implement the Get method in terms of attr syntax in order to
# give to the users an easy way to get objects from TDirectoryFile instances when
# only their key name is available (we think that the getattr(myDir, myKeyName) is
# a bit more awkward than myDir.Get("myKeyName"))

# TDirectoryFile::Get method, based on the attr syntax
def _TDirectoryFile_Get(self, objName):
    return getattr(self, objName)

# Pythonizor function
@pythonization()
def pythonize_tfile(klass, name):

    if name == 'TDirectoryFile':
       klass.Get = _TDirectoryFile_Get

    return True
