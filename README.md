# Clashdome Struggle
NFTs tournaments
 
# Build

```cd <smart_contract_directory>
git clone
go to directory
md build
cd build
cmake ..
make
```

# Update contract

While testing in testnet:
cleos -u https://testnet.waxsweden.org set contract clashdomestg ./clashdomestg -p clashdomestg@active

In production:
cleos -u https://api.waxsweden.org set contract clashdomestg ./clashdomestg -p clashdomestg@active

## License

[MIT](./LICENSE)

