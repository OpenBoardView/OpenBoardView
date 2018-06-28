#!/bin/sh
pushd "${1}/glad" > /dev/null
python -m glad --out-path="${2}" --profile="compatibility" --api="gl=3.2,gles2=2.0" --generator="c" --spec="gl" --no-loader --extensions="GL_EXT_texture_compression_s3tc,GL_OES_element_index_uint"
popd > /dev/null
