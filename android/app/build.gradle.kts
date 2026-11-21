import java.util.Properties

plugins {
    id("com.android.application")
}

val repoDir: File = rootDir.parentFile          // .../th01-port
val portDir: File = repoDir.parentFile          // workspace root
val thirdPartyDir: File = File(portDir, "third_party")

fun prop(name: String): String? = (findProperty(name) as String?)?.takeIf { it.isNotBlank() }

val abiList = (prop("th01.abis") ?: "arm64-v8a").split(',').map { it.trim() }.filter { it.isNotEmpty() }
val appVersion = prop("th01.versionName") ?: "1.3.0"
val appChannel = prop("th01.channel") ?: "release"

fun keyConfig(): Map<String, Any> {
    val ks = File(portDir, "secrets/debug.keystore")
    if (!ks.isFile) {
        ks.parentFile.mkdirs()
        val p = ProcessBuilder(
            "keytool", "-genkeypair", "-keystore", ks.path,
            "-storepass", "android", "-keypass", "android",
            "-alias", "th01debug", "-keyalg", "RSA", "-keysize", "2048",
            "-validity", "10000", "-dname", "CN=th01 debug"
        ).redirectErrorStream(true).start()
        p.inputStream.readBytes()
        if (p.waitFor() != 0) throw GradleException("keytool failed for $ks")
    }
    return mapOf("file" to ks, "storePass" to "android", "alias" to "th01debug", "keyPass" to "android")
}

android {
    namespace = "com.zun.th01"
    compileSdk = 36
    ndkVersion = "27.1.12297006"

    defaultConfig {
        applicationId = "com.zun.th01"
        minSdk = 26
        targetSdk = 36
        versionCode = 10031
        versionName = appVersion
        ndk { abiFilters += abiList }
        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DTH01_ANDROID=ON",
                    "-DANDROID_STL=c++_static",
                    "-DANDROID_PLATFORM=android-26"
                )
                cppFlags += listOf("-fno-exceptions", "-fno-rtti")
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../th01-port/CMakeLists.txt")
        }
    }

    signingConfigs {
        create("th01") {
            val k = keyConfig()
            storeFile = k["file"] as File
            storePassword = k["storePass"] as String
            keyAlias = k["alias"] as String
            keyPassword = k["keyPass"] as String
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName("th01")
            if (appChannel != "release") applicationIdSuffix = ".$appChannel"
            resValue("string", "app_name", "東方靈異伝 ～ Highly Responsive to Prayers")
        }
        debug {
            applicationIdSuffix = ".debug"
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName("th01")
            resValue("string", "app_name", "東方靈異伝 debug")
        }
    }

    buildFeatures {
        buildConfig = true
        resValues = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    sourceSets {
        getByName("main") {
            java.srcDir(File(thirdPartyDir, "SDL2/android-project/app/src/main/java"))
            assets.srcDir(File(projectDir, "src/main/assets"))
        }
    }

    packaging { jniLibs { useLegacyPackaging = false } }
    androidResources { noCompress += listOf("pf", "grp", "grz", "grf", "dat", "ptn", "bos", "grc", "mdt", "mde", "ogg") }
    lint { checkReleaseBuilds = false; abortOnError = false }
}

dependencies { }
