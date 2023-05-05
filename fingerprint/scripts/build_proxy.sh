cd ../proxy || exit 1

export PATH=$PATH:/usr/local/go/bin
go mod init proxy
go mod tidy
go install \
    github.com/grpc-ecosystem/grpc-gateway/v2/protoc-gen-grpc-gateway \
    github.com/grpc-ecosystem/grpc-gateway/v2/protoc-gen-openapiv2 \
    google.golang.org/protobuf/cmd/protoc-gen-go \
    google.golang.org/grpc/cmd/protoc-gen-go-grpc

cd ..

export GOROOT=/usr/local/go
export GOPATH=$HOME/go
export GOBIN=$GOPATH/bin
export PATH=$PATH:$GOROOT:$GOPATH:$GOBIN

protoc -I . \
    --go_out proxy/gen --go_opt paths=source_relative \
    --go-grpc_out proxy/gen --go-grpc_opt paths=source_relative \
    proto/fingerprint.proto
protoc -I . --grpc-gateway_out proxy/gen \
    --grpc-gateway_opt logtostderr=true \
    --grpc-gateway_opt paths=source_relative \
   	--grpc-gateway_opt generate_unbound_methods=true \
    proto/fingerprint.proto