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
      PACKAGE_DIR = "packages"
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

    stage('[UbuntuVM] Checkout Git TAG') {
      agent { label 'UbuntuVM' }
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
      when { expression { return params.BUILD_MACOSX }}
      steps {
        sh '''
        mkdir ${PACKAGE_DIR}

        ROOT="build"
        tar -czvf ${PACKAGE_DIR}/${PACKAGE_NAME}-${LABEL}-macosx${MACOSX_DEPLOYMENT_TARGET}-x86-64.tgz --strip-components 2 ${ROOT}/libs/*
        tar -czvf ${PACKAGE_DIR}/${PACKAGE_NAME}-${LABEL}-examples-macosx${MACOSX_DEPLOYMENT_TARGET}-x86-64.tgz --strip-components 2 ${ROOT}/examples/*
        '''
      }
    }

    stage('[UbuntuVM] Linux crosscompiled build') {
      agent { label 'UbuntuVM' }
      when { expression { return params.BUILD_LINUX_CROSS }}
      steps {
        sh '''
        make clean
        make crossbuilds
        '''
      }
    }

    stage('[UbuntuVM] Linux crosscompiled packages') {
      agent { label 'UbuntuVM' }
      when { expression { return params.BUILD_LINUX_CROSS }}
      steps {
        sh '''        
        mkdir ${PACKAGE_DIR}

        LIBNAME="libzenohpico"
        ROOT="crossbuilds"
        TARGETS=$(ls $ROOT)
        for TGT in $TARGETS; do
            echo "=== $TGT ==="

            tar -czvf ${PACKAGE_DIR}/${PACKAGE_NAME}-${LABEL}-${TGT}.tgz --strip-components 3 ${ROOT}/${TGT}/libs/*
            tar -czvf ${PACKAGE_DIR}/${PACKAGE_NAME}-${LABEL}-examples-${TGT}.tgz --strip-components 3 ${ROOT}/${TGT}/examples/*

            ARCH=$(echo $TGT | cut -d'-' -f2)
            DEBS=$(ls $ROOT/$TGT/${PACKAGE_DIR}/ | grep "deb" | grep -v "${LIBNAME}-dev" | grep -v "md5")  
            for D in $DEBS; do
                cp $ROOT/$TGT/${PACKAGE_DIR}/$D ${PACKAGE_DIR}/${PACKAGE_NAME}-${LABEL}-${ARCH}.deb
            done

            RPMS=$(ls $ROOT/$TGT/${PACKAGE_DIR}/ | grep "rpm" | grep -v "${LIBNAME}-dev" | grep -v "md5")
            for R in $RPMS; do
                cp $ROOT/$TGT/${PACKAGE_DIR}/$R ${PACKAGE_DIR}/${PACKAGE_NAME}-${LABEL}-${ARCH}.rpm
            done
        done

        cd ${PACKAGE_DIR}
        dpkg-scanpackages --multiversion . > Packages
        cat Packages
        gzip -c9 < Packages > Packages.gz
        rm Packages
        '''
      }
    }

    stage('[MacMini] Publish zenoh-pico MacOS X packages to download.eclipse.org') {
      when { expression { return params.PUBLISH_ECLIPSE_DOWNLOAD && params.BUILD_MACOSX }}
      steps {
        sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
          sh '''
            ssh ${HTTP_USER}@${HTTP_HOST} mkdir -p ${HTTP_DIR}/${LABEL}
            scp ${PACKAGE_DIR}/* ${HTTP_USER}@${HTTP_HOST}:${HTTP_DIR}/${LABEL}/
          '''
        }
      }
    }

    stage('[UbuntuVM] Publish zenoh-pico Linux crosscompiled packages to download.eclipse.org') {
      agent { label 'UbuntuVM' }
      when { expression { return params.PUBLISH_ECLIPSE_DOWNLOAD && params.BUILD_LINUX_CROSS }}
      steps {
        sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
          sh '''
            ssh ${HTTP_USER}@${HTTP_HOST} mkdir -p ${HTTP_DIR}/${LABEL}
            scp ${PACKAGE_DIR}/* ${HTTP_USER}@${HTTP_HOST}:${HTTP_DIR}/${LABEL}/
          '''
        }
      }
    }
  }
}

def get_label() {
    return env.GIT_TAG.startsWith('origin/') ? env.GIT_TAG.minus('origin/') : env.GIT_TAG
}
