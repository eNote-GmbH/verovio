#!/bin/bash

# This script will build a universal framework for Xcode.
# It will contain libraries for iOS (device, simulator) for arm64 and x84_64, as well as a library for macOS for both arm64 and x86_64.

TMP_DIR="./tmp"
IOS_SIM_ARCHIVE="$TMP_DIR/iOS-Simulator.xcarchive"
IOS_ARCHIVE="$TMP_DIR/iOS.xcarchive"
MACOS_ARCHIVE="$TMP_DIR/macOS.xcarchive"
XCFRAMEWORK="$TMP_DIR/VerovioFramework.xcframework"
XCFRAMEWORK_ZIP="$XCFRAMEWORK.zip"
VEROVIO_COMMIT_HASH="$(git rev-parse --short HEAD)"
AWS_PUBLIC_LINK="https://enote-macos-app.s3.eu-central-1.amazonaws.com/verovio/$VEROVIO_COMMIT_HASH.zip"
VEROVIO_VERSION="$(cat codemeta.json | jq .softwareVersion | sed -e "s/-dev//g" | sed -e "s/\"//g" | sed -e "s/.[0-9]$//g" )"

# Emergency exit function
function die {
    echo $1
    exit -1
}

# Create paths and setup environment
function clear_tmp_dir {
    if [ -d "$TMP_DIR" ]; then
        rm -rf "$TMP_DIR" || die "Can't clear $TMP_DIR"
    fi
    mkdir "$TMP_DIR" || die "Can't create $TMP_DIR"
}

function build_ios_simulator {
    xcodebuild archive \
        -scheme VerovioFramework \
        -archivePath "$IOS_SIM_ARCHIVE" \
        -sdk iphonesimulator SKIP_INSTALL=NO | xcpretty || die "Can't build iOS Simulator Framework"
}

function build_ios {
    xcodebuild archive \
        -scheme VerovioFramework \
        -archivePath "$IOS_ARCHIVE" \
        -sdk iphoneos SKIP_INSTALL=NO | xcpretty || die "Can't build iOS Framework"
}

function build_macos {
    xcodebuild archive \
        -scheme VerovioFramework \
        -archivePath "$MACOS_ARCHIVE" \
        -sdk macosx SKIP_INSTALL=NO | xcpretty || die "Can't build macOS Framework"
}

function build_xcframework {
    xcodebuild -create-xcframework \
        -framework "$IOS_SIM_ARCHIVE/Products/Library/Frameworks/VerovioFramework.framework" \
        -framework "$IOS_ARCHIVE/Products/Library/Frameworks/VerovioFramework.framework" \
        -framework "$MACOS_ARCHIVE/Products/Library/Frameworks/VerovioFramework.framework" \
        -output "$XCFRAMEWORK" || die "Can't combine XCFramework"
}

function compress_xcframework {
    pushd tmp
    zip -r "VerovioFramework.xcframework.zip" "VerovioFramework.xcframework" || die "Can't compress xcframework"
    popd
}

function upload_to_s3 {
    aws s3 cp "$XCFRAMEWORK_ZIP" "s3://enote-macos-app/verovio/$VEROVIO_COMMIT_HASH.zip" || die "Can't upload to S3"
    echo "$AWS_PUBLIC_LINK"
}

function update_swift_package {
    # clone swift package
    PROJ_DIR="$(pwd)"
    TMP="$(mktemp -d)"
    cd "$TMP"
    git clone git@github.com:eNote-GmbH/Verovio-XCFramework.git

    # generate checksum for zipped framework
    cd Verovio-XCFramework
    cp "$PROJ_DIR/$XCFRAMEWORK_ZIP" ./
    CHECKSUM="$(swift package compute-checksum VerovioFramework.xcframework.zip)"
    rm VerovioFramework.xcframework.zip

    # modify Package.swift
    cat Package.template | sed -e "s|ZIP_URL|$AWS_PUBLIC_LINK|g" | sed -e "s/ZIP_CHECKSUM/$CHECKSUM/g" > Package.swift

    # calculate tag for swit package manager
    if [ ! -d versions ]; then
        mkdir versions
    fi

    pushd versions
    if [ ! -f "$VEROVIO_VERSION" ]; then
        echo "0" > "$VEROVIO_VERSION"
    fi
    PREV_BUILD_NR="$(cat "$VEROVIO_VERSION")"
    BUILD_NR="$(bc <<< "$PREV_BUILD_NR + 1")"
    echo $BUILD_NR > "$VEROVIO_VERSION"
    popd

    # commit changes to swift package
    git add .
    git commit -am "Binary framework for verovio commit $VEROVIO_COMMIT_HASH"
    git push origin master

    # Add tag with verovio commit hash 
    git tag "verovio-$VEROVIO_COMMIT_HASH"
    git tag "v$VEROVIO_VERSION.$BUILD_NR"

    # push tags
    git push --tags
    cd "$PROJ_DIR"
    rm -rf "$TMP"
}

# Entry point
function main {
    # clear_tmp_dir
    # build_ios_simulator
    # build_ios
    # build_macos
    # build_xcframework
    # compress_xcframework
    # upload_to_s3
    update_swift_package
}

main