This directory contains the code and confiuguration files to build a simple CSPOT application.

Step 1: clone the CSPOT repo and checkout the caplets branch

  -- git clone https://github.com/MAYHEM-Lab/cspot.git
  -- git checkout caplets

Step 2: create an application build directory and copy the test application
        directory contents from the repo

  -- mkdir my_application_build
  -- cp -r cspot/examples/cspot-example-app-with-cmake/* ./my_application_build

Step 3: cd into the application build directory and run the build script

  -- cd ./my_application_build
  -- ./build-cspot-example-app.sh

The script will fetch the latest daily binary library release, build the
app in ./my_application_build/build/bin, install the daily binary runtime,
start it, runt he app, and shut down the runtime.  
