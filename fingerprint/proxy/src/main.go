package main

import (
    "os"
	"context"
	"fmt"
	"log"
	"net"
	"net/http"
	gen "proxy/gen/proto"

	"github.com/grpc-ecosystem/grpc-gateway/v2/runtime"
	"google.golang.org/grpc"
)

func main() {
    servAddr := os.Getenv("FINGERPRINT_ADDRESS")
    servPort := os.Getenv("FINGERPRINT_PORT")

    if servAddr == "" || servPort == "" {
    	log.Fatal("FINGERPRINT_ADDRESS and FINGERPRINT_PORT must be set")
    }
    servAddr = servAddr + ":" + servPort

    proxyPort := os.Getenv("GRPC_PROXY_PORT")
    if proxyPort == "" {
    	log.Fatal("GRPC_PROXY_PORT is not defined")
    }
    proxyPort = ":" + proxyPort

    mux := runtime.NewServeMux()

	err := gen.RegisterSirenFingerprintHandlerFromEndpoint(context.Background(), mux, servAddr, []grpc.DialOption{grpc.WithInsecure()})
	if err != nil {
        log.Fatal(err)
    }

    server := http.Server {
        Handler: mux,
    }

	l, err := net.Listen("tcp", proxyPort)
	fmt.Println("Proxy listening on port", proxyPort)

	if err != nil {
		log.Fatal(err)
	}

	err = server.Serve(l)
	if err != nil {
		log.Fatal(err)
	}
}