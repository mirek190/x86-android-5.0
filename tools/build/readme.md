## What is this?

The official Gradle plugin to build Android applications.

## DSL

The plugin adds several concepts to the Gradle DSL, all in the android extension:

* A _default config_. This lets you configure default values for your application.
* A _product flavor_. For example, a free or a paid-for flavour.
* A _build type_. There are 2 predefined build types, called `release` and `debug`. You can add additional build types.

If you do not define any flavors for your product, only the values from the default config are used. Flavor settings override the default config.

From this, the plugin will add the appropriate tasks to build each combination of build type and product flavor. The
plugin will also define the following source directories:

* `src/main/java` - Java source to be included in all application variants.
* `src/main/res` - Resources to be included in all application variants.
* `src/main/AndroidManifest.xml' - The application manifest (currently shared by all application variants).
* `src/$BuildType/java` - Java source to be included in all application variants with the given build type.
* `src/$BuildType/res` - Java source to be included in all application variants with the given build type.
* `src/$ProductFlavor/java` - Resources to be included in all application variants with the given product flavor.
* `src/$ProductFlavor/res` - Resources to be included in all application variants with the given product flavor.
* `src/test/java` - Test source to be included in all test applications.
* `src/test$ProductFlavor/java` - Test source to be include for the test application for the given product flavor.

You can configure these locations by configuring the associated source set provided by the android extension.

Compile time dependencies are declared in the usual way.

Have a look at the `tests/basic/build.gradle` build file and other projects in tests `customized/build.gradle` to see the DSL in action.

### Configuration options

* `android.target` - required.
* `android.defaultConfig.versionCode` - defaults to that specified in `src/main/AndroidManifest.xml`
* `android.defaultConfig.versionName` - defaults to that specified in `src/main/AndroidManifest.xml`
* `android.productFlavors.$flavor.packageName` - defaults to that specified in `src/main/AndroidManifest.xml`
* `android.productFlavors.$flavor.versionCode` - defaults to `${android.defaultConfig.versionCode}`
* `android.productFlavors.$flavor.versionName` - defaults to `${android.defaultConfig.versionName}`
* `android.buildTypes.$type.zipAlign` - defaults to `true` for `release` and `false` for `debug`
* `android.sourceSets.main.java.srcDirs` - defaults to `src/main/java`
* `android.sourceSets.main.resources.srcDirs` - defaults to `src/main/res`
* `android.sourceSets.$flavor.java.srcDirs` - defaults to `src/$flavor/java`
* `android.sourceSets.$flavor.resources.srcDirs` - defaults to `src/$flavor/res`
* `android.sourceSets.$buildType.java.srcDirs` - defaults to `src/$buildType/java`
* `android.sourceSets.$buildType.resources.srcDirs` - defaults to `src/$buildType/res`
* `android.sourceSets.test.java.srcDirs` - defaults to `src/test/java`
* `android.sourceSets.test$Flavor.java.srcDirs` - defaults to `src/test$Flavor/java`
* `dependencies.compile` - compile time dependencies for all applications.

## Contents

The source tree contains the following:

* The `builder` directory contains the builder library.
* The `gradle` directory contains the plugin implementation.
* The `tests` directory contains various test applications used both as samples and tests by the plugin build system.

## Usage

To build the plugin, run `./gradlew uploadArchives`

To build the plugin for release, removing the SNAPSHOT from the version, run `./gradlew --init-script release.gradle <tasks>`

To import the plugin into the IDE, run `./gradlew idea` or `./gradlew eclipse`.

To build a test application:
1. cd into the root directory of the test application.
2. Edit the `local.properties` file to point at your local install of the Android SDK. Normally, these files would not
be checked into source control, but would be generated when the project is bootstrapped.
3. Run `../../gradlew tasks` to see the tasks that are available.

You can also run these tasks:

* `assemble` - builds all combinations of build type and product flavor
* `assemble$BuildType` - build all flavors for the given build type.
* `assemble$ProductFlavor` - build all build types for the given product flavor.
* `assemble$ProductFlavor$BuildType` - build the given application variant.
* `install$ProductFlavor$BuildType` - build and install the given application variant.

## Implementation

For each variant (product-flavor, build-type):

* Generates resource source files into `build/source` from resource directories (main-source-set, product-flavor-source-set, build-type-source-set)
* Compile source files (main-source-set, product-flavor-source-set, build-type-source-set, generated-source).
* Converts the bytecode into `build/libs`
* Crunches resources in `build/resources`
* Packages the resource into `build/libs`
* Assembles the application package into `build/libs`.
