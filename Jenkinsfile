pipeline {
  agent { label 'MacMini' }
  options { skipDefaultCheckout() }
  parameters {
    gitParameter(name: 'GIT_TAG',
                 type: 'PT_BRANCH_TAG',
                 description: 'The Git tag to checkout. If not specified "master" will be checkout.',
                 defaultValue: 'master')
    booleanParam(name: 'BUILD_MACOSX',
                 description: 'Build MacOS X target.',
                 defaultValue: true)
    booleanParam(name: 'BUILD_LINUX_CROSS',
                 description: 'Build Linux crosscompiled targets.',
                 defaultValue: true)
    booleanParam(name: 'PUBLISH_ECLIPSE_DOWNLOAD',
                 description: 'Publish the resulting artifacts to Eclipse download.',
                 defaultValue: false)
  }

  environment {
      PACKAGE_NAME = "eclipse-zenoh-pico"
      LABEL = get_label()
      MACOSX_DEPLOYMENT_TARGET=10.7
      HTTP_USER = "genie.zenoh"
      HTTP_HOST = "projects-storage.eclipse.org"
      HTTP_DIR = "/home/data/httpd/download.eclipse.org/zenoh/zenoh-pico/"
  }

  stages {
    stage('[MacMini] Checkout Git TAG') {
      steps {
        deleteDir()
        checkout([$class: 'GitSCM',
                  branches: [[name: "${params.GIT_TAG}"]],
                  doGenerateSubmoduleConfigurations: false,
                  extensions: [],
                  gitTool: 'Default',
                  submoduleCfg: [],
                  userRemoteConfigs: [[url: 'https://github.com/eclipse-zenoh/zenoh-pico.git']]
                ])
      }
    }

    stage('[MacMini] MacOS build') {
      when { expression { return params.BUILD_MACOSX }}
      steps {
        sh '''
        make all
        '''
      }
    }

    stage('[MacMini] MacOS package') {
      when { expression { return params.PUBLISH_ECLIPSE_DOWNLOAD && params.BUILD_MACOSX }}
      steps {
        sh '''
        mkdir packages

        ROOT="build"
        tar -czvf packages/${PACKAGE_NAME}-${LABEL}-macosx${MACOSX_DEPLOYMENT_TARGET}-x86-64.tgz --strip-components 2 ${BUILD}/libs/*
        tar -czvf packages/${PACKAGE_NAME}-${LABEL}-examples-macosx${MACOSX_DEPLOYMENT_TARGET}-x86-64.tgz --strip-components 2 ${BUILD}/examples/*
        '''
      }
    }

    stage('[MacMini] Linux crosscompiled build') {
      when { expression { return params.BUILD_LINUX_CROSS }}
      steps {
        sh '''
        make crossbuilds
        '''
      }
    }

    stage('[MacMini] Linux crosscompiled packages') {
      when { expression { return params.PUBLISH_ECLIPSE_DOWNLOAD && params.BUILD_LINUX_CROSS }}
      steps {
        sh '''
        mkdir packages

        LIBNAME="libzenohpico"
        ROOT="crossbuilds"
        TARGETS=$(ls $ROOT)
        for TGT in $TARGETS; do
            echo "=== $TGT ==="

            tar -czvf packages/${PACKAGE_NAME}-${LABEL}-${TGT}.tgz --strip-components 3 $ROOT/$TGT/libs/*
            tar -czvf packages/${PACKAGE_NAME}-${LABEL}-examples-${TGT}.tgz --strip-components 3 $ROOT/$TGT/examples/*

            ARCH=$(echo $TGT | cut -d'-' -f2)
            DEBS=$(ls $ROOT/$TGT/packages/ | grep "deb" | grep -v "${LIBNAME}-dev" | grep -v "md5")  
            for D in $DEBS; do
                cp $ROOT/$TGT/packages/$D packages/${PACKAGE_NAME}-${LABEL}_${ARCH}.deb
            done

            RPMS=$(ls $ROOT/$TGT/packages/ | grep "rpm" | grep -v "${LIBNAME}-dev" | grep -v "md5")
            for R in $RPMS; do
                cp $ROOT/$TGT/packages/$R packages/${PACKAGE_NAME}-${LABEL}_${ARCH}.rpm
            done
        done
        '''
      }
    }

    stage('[MacMini] Publish zenoh-pico packages to download.eclipse.org') {
      when { expression { return params.PUBLISH_ECLIPSE_DOWNLOAD && params.BUILD_LINUX64 }}
      steps {
        sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
          sh '''
            ssh ${HTTP_USER}@${HTTP_HOST} mkdir -p ${HTTP_DIR}/${LABEL}
            scp packages/* ${HTTP_USER}@${HTTP_HOST}:${HTTP_DIR}/${LABEL}/
          '''
        }
      }
    }

    stage('[UbuntuVM] Build Packages.gz for download.eclipse.org') {
      agent { label 'UbuntuVM' }
      when { expression { return params.PUBLISH_ECLIPSE_DOWNLOAD && params.BUILD_LINUX_CROSS }}
      steps {
        deleteDir()
        sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
          sh '''
          scp ${HTTP_USER}@${HTTP_HOST}:${HTTP_DIR}/${LABEL}/*.deb ./
          dpkg-scanpackages --multiversion . > Packages
          cat Packages
          gzip -c9 < Packages > Packages.gz
          scp Packages.gz ${HTTP_USER}@${HTTP_HOST}:${HTTP_DIR}/${LABEL}/
          '''
        }
      }
    }
  }
}

def get_label() {
    return env.GIT_TAG.startsWith('origin/') ? env.GIT_TAG.minus('origin/') : env.GIT_TAG
}
