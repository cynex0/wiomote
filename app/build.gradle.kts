plugins {
    id("com.android.application")
}

android {
    namespace = "se.gu.wiomote"
    compileSdk = 34

    defaultConfig {
        applicationId = "se.gu.wiomote"
        minSdk = 28
        targetSdk = 34
        versionCode = 1
        versionName = "0.1"
    }

    signingConfigs {
        create("release") {
            storeFile = file(System.getenv("ANDROID_KEY_STOREFILE") ?: "../keystore-release.jks")
            storePassword = System.getenv("ANDROID_KEYSTORE_PASSWORD")
            keyAlias = System.getenv("ANDROID_KEY_ALIAS")
            keyPassword = System.getenv("ANDROID_KEYSTORE_PASSWORD")
        }
    }

    buildTypes {
        getByName("release") {
            isMinifyEnabled = true
            isDebuggable = false
            signingConfig = signingConfigs.getByName("release")
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
        getByName("debug") {
            applicationIdSuffix = ".debug"
            isMinifyEnabled = false
            isDebuggable = true
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    packaging {
        resources {
            excludes.add("META-INF/INDEX.LIST")
            excludes.add("META-INF/io.netty.versions.properties")
        }
    }
}

dependencies {
    implementation("androidx.appcompat:appcompat:1.6.1")
    implementation("com.readystatesoftware.sqliteasset:sqliteassethelper:2.0.1")
    implementation("com.github.shuhart:stickyheader:1.1.0")
    implementation("com.github.rubensousa:gravitysnaphelper:2.2.2")
    implementation("com.google.android.material:material:1.12.0")
    implementation("com.google.android.gms:play-services-location:21.2.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    implementation("com.hivemq:hivemq-mqtt-client:1.3.3")
}