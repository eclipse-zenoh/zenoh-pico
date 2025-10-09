//
// Copyright (c) 2025 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_LINK_TLS == 1

static const char SERVER_CA_PEM[] =
    "LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0tCk1JSURTekNDQWpPZ0F3SUJBZ0lJVGN3djFOMTBu"
    "cUV3RFFZSktvWklodmNOQVFFTEJRQXdJREVlTUJ3R0ExVUUKQXhNVmJXbHVhV05oSUhKdmIzUWdZ"
    "MkVnTkdSall6Sm1NQ0FYRFRJek1ETXdOakUyTkRFd05sb1lEekl4TWpNdwpNekEyTVRZME1UQTJX"
    "akFnTVI0d0hBWURWUVFERXhWdGFXNXBZMkVnY205dmRDQmpZU0EwWkdOak1tWXdnZ0VpCk1BMEdD"
    "U3FHU0liM0RRRUJBUVVBQTRJQkR3QXdnZ0VLQW9JQkFRQzJXVWdON05NbFhJa25ldzFjWGlUV0dt"
    "UzAKMVQxRWpjTk5EQXE3RHFaNy9aVlhyakQ0N3l4VHQ1RU9pT1hLL2NJTktOdzRacS9NS1F2cTlx"
    "dStPYXg0bHdpVgpIYTBpOFNoR0xTdVlJMUhCbFh1NE1tdmRHKzMvU2p3WW9Hc0dhU2hyMHkvUUd6"
    "RDNjRCtEUVpnL1JhYUlQSGxPCk1kbWlVWHhrTWN5NHFhMGhGSjFpbWxKZHEvNlRseDQ2WCswdlJD"
    "aDhua2Vrdk9aUit0N1o1VTRqbjRYRTU0S2wKMFBpd2N5WDh2ZkRaM2VwYS9GU0hadlZRaWVNL2c1"
    "WWg5T2pJS0NrZFdSZzd0RDBJRUdzYVcxMXRFUEo1U2lRcgptRHFkUm5lTXpaS3FZMHhDK1FxWFN2"
    "SWx6cE9qaXU4UFlReDd4dWdhVUZFL25wS1JRZHZoOG9qSEpNZE5BZ01CCkFBR2pnWVl3Z1lNd0Rn"
    "WURWUjBQQVFIL0JBUURBZ0tFTUIwR0ExVWRKUVFXTUJRR0NDc0dBUVVGQndNQkJnZ3IKQmdFRkJR"
    "Y0RBakFTQmdOVkhSTUJBZjhFQ0RBR0FRSC9BZ0VBTUIwR0ExVWREZ1FXQkJUWDQ2K3ArUG8xbnBF"
    "NgpRTFE3bU1JKzgzczZxREFmQmdOVkhTTUVHREFXZ0JUWDQ2K3ArUG8xbnBFNlFMUTdtTUkrODNz"
    "NnFEQU5CZ2txCmhraUc5dzBCQVFzRkFBT0NBUUVBYU4wSXZFQzY3N1BML0pYek1yWGN5QlY4OEl2"
    "aW1sWU4wekN0NDhHWWxobXgKdkwxWVVERkxKRUI3SitkeUVSR0U1TjZCS0tER2JsQzRXaVRGZ0RN"
    "TGNIRnNNR1JjMHY3ektQRjFQU0J3UllKaQp1YkFta3dkdW5HRzVwRFBVWXRURURQWE1sZ0NsWjBZ"
    "eXFTRkpNT3FBNEl6UWc2ZXhWalh0VXhQcXp4Tmh5QzdTCnZsZ1V3UGJYNDZ1Tmk1ODFhOStMczJW"
    "M2ZnMFpuaGtUU2N0WVpIR1pOZWgwTnNmN0FtOHhkVURZRy9iWmNWZWYKamJROWdwQ2hvc2RqRjBC"
    "Z2JsbzdIU1VjdC8yVmErWWxZd1crV0ZqSlg4azRvTjZaVTVXNXhoZGZPOEN6bWd3awpVUzVrSi8r"
    "MU0wdVI4elVoWkhMNjFGYnNkUHhFaitmWUtySHY0d29vK0E9PQotLS0tLUVORCBDRVJUSUZJQ0FU"
    "RS0tLS0tCg==";

static const char SERVER_CERT_PEM[] =
    "LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0tCk1JSURMakNDQWhhZ0F3SUJBZ0lJVzFtQXRKV0pB"
    "Sll3RFFZSktvWklodmNOQVFFTEJRQXdJREVlTUJ3R0ExVUUKQXhNVmJXbHVhV05oSUhKdmIzUWdZ"
    "MkVnTkdSall6Sm1NQ0FYRFRJek1ETXdOakUyTkRFd05sb1lEekl4TWpNdwpNekEyTVRZME1UQTJX"
    "akFVTVJJd0VBWURWUVFERXdsc2IyTmhiR2h2YzNRd2dnRWlNQTBHQ1NxR1NJYjNEUUVCCkFRVUFB"
    "NElCRHdBd2dnRUtBb0lCQVFDWU1MSktvb2MrWVJsS0VNZmVWMDlwWDlteUgzNGVVY1V1VDBmWFM4"
    "bG0KUGxaL05XN21tNWxEd2E4RVVnNjFXdVhRdjJvdVFEcHRtSWNkZWIvdzRSVzkzWGZsa3luZzFY"
    "YmQ5MU93UUJKZAorOFpWQmp6TDdoU1JrM1FQRHF4L0NWQlUvSTFHbVhLemI2Y1d6cTFmVGtPbjFX"
    "TE5YZjIxSTZwNytOM3FITFBGCkpRZW9WcTFIQkJGY0FqVGdKbnB5UU52UkdMRHVMVEsrT3NXRUdp"
    "YjJVOHFyZ2lSZGthQkxreEdYU2xHQUJsT28KY1F5Vy96T2hmNHB3YjJaL0pBZ2UybVJXNUljZXhD"
    "UEJXaW50OHlkUHNvSkRkczhqNStBeVlDRDZIVWhIWDBPYgpRa3o3M09XN2YyUFFodVRLMnV6S3kw"
    "WXo2bE5GdDJudXphV0MwNHdJVzNUN0FnTUJBQUdqZGpCME1BNEdBMVVkCkR3RUIvd1FFQXdJRm9E"
    "QWRCZ05WSFNVRUZqQVVCZ2dyQmdFRkJRY0RBUVlJS3dZQkJRVUhBd0l3REFZRFZSMFQKQVFIL0JB"
    "SXdBREFmQmdOVkhTTUVHREFXZ0JUWDQ2K3ArUG8xbnBFNlFMUTdtTUkrODNzNnFEQVVCZ05WSFJF"
    "RQpEVEFMZ2dsc2IyTmhiR2h2YzNRd0RRWUpLb1pJaHZjTkFRRUxCUUFEZ2dFQkFBeHJtUVBHNTR5"
    "YktnTVZsaU44Ck1nNXBvdlNkUElWVm5sVS9IT1ZHOXl4ekFPYXYveFFQMDAzTTR3cXBhdFd4STh0"
    "UjFQY0x1WmYwRVBtY2RKZ2IKdFZsOW5aTVZadHZlUW5ZTWxVOFBwa0VWdTU2Vk00WnIzckg5bGlQ"
    "UmxyMEpFQVhPRGRLdzc2a1dLem1kcVdaLwpyemh1cDNFazdpRVg2VDVqL2NQVXZUV3RNRDRWRUsy"
    "STdmZ29LU0hJWDhNSVZ6cU03Y3Vib0dXUHRTM2VSTlhsCk1ndmFoQTRUd0xFWFBFZStWMVdBcTZu"
    "U2I0ZzJxU1hXSURwSXN5L08xV0dTL3p6Um5Ldlh1OS85TmtYV3FaTWwKQzFMU3BpaVFVYVJTZ2xP"
    "dllmL1p4NnIrNEJPUzRPYWFBcndIa2VjWlFxQlNDY0JMRUF5Yi9GYWFYZEJvd0kwVQpQUTQ9Ci0t"
    "LS0tRU5EIENFUlRJRklDQVRFLS0tLS0K";

static const char SERVER_KEY_PEM[] =
    "LS0tLS1CRUdJTiBSU0EgUFJJVkFURSBLRVktLS0tLQpNSUlFcEFJQkFBS0NBUUVBbURDeVNxS0hQ"
    "bUVaU2hESDNsZFBhVi9ac2g5K0hsSEZMazlIMTB2SlpqNVdmelZ1CjVwdVpROEd2QkZJT3RWcmww"
    "TDlxTGtBNmJaaUhIWG0vOE9FVnZkMTM1Wk1wNE5WMjNmZFRzRUFTWGZ2R1ZRWTgKeSs0VWtaTjBE"
    "dzZzZndsUVZQeU5ScGx5czIrbkZzNnRYMDVEcDlWaXpWMzl0U09xZS9qZDZoeXp4U1VIcUZhdApS"
    "d1FSWEFJMDRDWjZja0RiMFJpdzdpMHl2anJGaEJvbTlsUEtxNElrWFpHZ1M1TVJsMHBSZ0FaVHFI"
    "RU1sdjh6Cm9YK0tjRzltZnlRSUh0cGtWdVNISHNRandWb3A3Zk1uVDdLQ1EzYlBJK2ZnTW1BZyto"
    "MUlSMTlEbTBKTSs5emwKdTM5ajBJYmt5dHJzeXN0R00rcFRSYmRwN3MybGd0T01DRnQwK3dJREFR"
    "QUJBb0lCQUROVFNPMnV2bG1sT1hnbgpES0RKWlRpdVlLYVh4RnJKVE94L1JFVXhnK3g5WFlKdExN"
    "ZU05alZKbnBLZ2NlRnJsRkhBSERrWTVCdU44eE5YCnVnbXNmejZXOEJaMmVRc2dNb1JOSXVZdjFZ"
    "SG9wVXlMVy9tU2cxRk5Iempzdy9QYjJrR3ZJcDRLcGdvcHYzb0wKbmFDa3JtQnRzSEorSGsvMmhV"
    "cGw5Y0U4aU13VldjVmV2THp5SGk5OGpOeTFJRGRJUGhSdGwwZGhNaXFDNU1Scgo0Z0xKNWdOa0xZ"
    "WDd4ZjN0dzVIbWZrL2JWTlByb3FaWERJUVZJN3JGdkl0WDU4Nm52UTNMTlFrbVcvRDJTaFpmCjNG"
    "RXFNdTZFZEEyWWNjNFVaZ0FsUU5HVjBWQnJXV1ZYaXpPUSs5Z2pMbkJrM2tKanFmaWdDVTZORzk0"
    "YlRKK0gKMFlJaHNHRUNnWUVBd2RTU3l1TVNPWGd6WlE3VnYrR3NObi83aXZpL0g4ZWIvbER6a3Nx"
    "Uy9Kcm9BMmNpQW1IRwoyT0YzMGVVSktSZytTVHFCVHBPZlhnUzRRVWE4UUxTd0JTbndjdzY1Nzl4"
    "OWJZR1VocUQyWXBhdzl1Q25PdWtBCkN3d2dnWjljRG1GMHRiNXJZanFrVzNiRlBxa0NuVEdiMHls"
    "TUZhWVJoUkRVMjBpRzV0OFBRY2tDZ1lFQXlRRU0KS0sxOEZMUVVLaXZHclFnUDVJYjZJQzNteXps"
    "SEd4RHpmb2JYR3BhUW50Rm5IWTdDeHAvNkJCdG1BU3p0OUp4dQpldG5yZXZtenJiS3FzTFRKU2cz"
    "aXZiaXEwWVRMQUoxRnNackNwNzFkeDQ5WVIvNW85UUZpcTBuUW9LbndVVmViCi9ockRqTUFva05r"
    "akZMNXZvdVhPNzExR1NTNll5TTRXekFLWkFxTUNnWUVBaHFHeGFHMDZqbUo0U0Z4NmliSWwKblNG"
    "ZVJoUXJKTmJQK21DZUhycklSOThOQXJnUy9sYU4rTHo3TGZhSlcxcjBnSWE3cENtVGk0bDV0aFY4"
    "MHZEdQpSbGZ3Sk9yNHFhdWNENER1K21nNVd4ZFNTZGlYTDZzQmxhclJ0VmRNYU15MmRUcVRlZ0pE"
    "Z1NoSkx4SFR0LzNxClAweXpCV0o1VHRUM0ZHMFhEcXVtL0VrQ2dZQVlOSHdXV2UzYlFHUTlQOUJJ"
    "L2ZPTC9ZVVpZdTJzQTFYQXVLWFoKMHJzTWhKMGR3dkc3NlhrakdoaXRiZTgyclFacXNudkxaM3Fu"
    "OEhIbXRPRkJMa1FmR3RUM0s4bkdPVXVJNDJlRgpIN0haS1VDbHkybENJaXpaZERWQmt6NEFXdmFK"
    "bFJjLzNsRTJIZDNFczZFNTJrVHZST1ZLaGR6MDZ4dVM4dDVqCjZ0d3FLUUtCZ1FDMDFBZWlXTDZS"
    "em8reVpOelZnYnBlZURvZ2FaejVkdG1VUkRnQ1lIOHlGWDVlb0NLTEhmbkkKMm5ESW9xcGFIWTBM"
    "dVgrZGludUgralA0dGx5bmRiYzJtdVhuSGQ5cjBhdHl0eEE2OWF5M3NTQTVXRnRmaTRlZgpFU0Vs"
    "R082cVhFQTgyMVJwUXArMit1aEw5MCtpQzI5NGNQcWxTNUxEbXZUTXlwVkRIenJ4UFE9PQotLS0t"
    "LUVORCBSU0EgUFJJVkFURSBLRVktLS0tLQo=";

static int received_count = 0;

static void data_handler(z_loaned_sample_t *sample, void *ctx) {
    (void)ctx;
    z_view_string_t key;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &key);
    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);
    printf(">> [Subscriber] Received ('%.*s': '%.*s')\n", (int)z_string_len(z_loan(key)), z_string_data(z_loan(key)),
           (int)z_string_len(z_loan(value)), z_string_data(z_loan(value)));
    z_drop(z_move(value));
    received_count++;
}

int main(int argc, char **argv) {
    char *keyexpr = "demo/example/**";
    int max_msgs = 0;
    bool has_listen = false;
    bool enable_mtls = false;
    bool enable_verify_name = false;
    const char *ca_path = NULL;
    const char *listen_key_path = NULL;
    const char *listen_cert_path = NULL;
    const char *client_key_path = NULL;
    const char *client_cert_path = NULL;

    z_owned_config_t config;
    z_config_default(&config);

    int opt;
    while ((opt = getopt(argc, argv, "hk:e:m:l:n:C:P:Q:R:S:MV")) != -1) {
        switch (opt) {
            case 'h':
                printf("Usage: %s [options]\n", argv[0]);
                printf("  -k <keyexpr>  Key expression filter (default: demo/example/**)\n");
                printf("  -n <count>    Stop after receiving <count> samples (default: infinite)\n");
                printf("  -e <locator>  Add connect locator (e.g., tls/127.0.0.1:7447)\n");
                printf("  -l <locator>  Set listen locator (e.g., tls/0.0.0.0:7447)\n");
                printf("  -m <mode>     Session mode: client or peer (default: client)\n");
                printf("  -C <file>     CA bundle path (optional; inline bundle used otherwise)\n");
                printf("  -P <file>     Listener private key path (peers)\n");
                printf("  -Q <file>     Listener certificate path (peers)\n");
                printf("  -R <file>     Client private key path (mTLS)\n");
                printf("  -S <file>     Client certificate path (mTLS)\n");
                printf("  -M            Enable mutual TLS\n");
                printf("  -V            Enable hostname verification\n");
                z_drop(z_move(config));
                return 0;
            case 'k':
                keyexpr = optarg;
                break;
            case 'e':
                if (zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, optarg) != Z_OK) {
                    fprintf(stderr, "Failed to add connect locator\n");
                    z_drop(z_move(config));
                    return -1;
                }
                break;
            case 'm':
                if (zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, optarg) != Z_OK) {
                    fprintf(stderr, "Failed to set session mode\n");
                    z_drop(z_move(config));
                    return -1;
                }
                break;
            case 'l':
                if (zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, optarg) != Z_OK) {
                    fprintf(stderr, "Failed to set listen locator\n");
                    z_drop(z_move(config));
                    return -1;
                }
                has_listen = true;
                break;
            case 'n':
                max_msgs = atoi(optarg);
                break;
            case 'C':
                ca_path = optarg;
                break;
            case 'P':
                listen_key_path = optarg;
                break;
            case 'Q':
                listen_cert_path = optarg;
                break;
            case 'R':
                client_key_path = optarg;
                break;
            case 'S':
                client_cert_path = optarg;
                break;
            case 'M':
                enable_mtls = true;
                break;
            case 'V':
                enable_verify_name = true;
                break;
            default:
                z_drop(z_move(config));
                return -1;
        }
    }

    if (ca_path) {
        if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_ROOT_CA_CERTIFICATE_KEY, ca_path) != Z_OK) {
            fprintf(stderr, "Failed to set CA certificate path\n");
            z_drop(z_move(config));
            return -1;
        }
    } else {
        if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_ROOT_CA_CERTIFICATE_BASE64_KEY, SERVER_CA_PEM) != Z_OK) {
            fprintf(stderr, "Failed to set inline CA certificate\n");
            z_drop(z_move(config));
            return -1;
        }
    }

    if (has_listen) {
        if (listen_key_path) {
            if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_LISTEN_PRIVATE_KEY_KEY, listen_key_path) != Z_OK) {
                fprintf(stderr, "Failed to set listener private key path\n");
                z_drop(z_move(config));
                return -1;
            }
        } else {
            if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_LISTEN_PRIVATE_KEY_BASE64_KEY, SERVER_KEY_PEM) !=
                Z_OK) {
                fprintf(stderr, "Failed to set inline listener private key\n");
                z_drop(z_move(config));
                return -1;
            }
        }

        if (listen_cert_path) {
            if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_LISTEN_CERTIFICATE_KEY, listen_cert_path) != Z_OK) {
                fprintf(stderr, "Failed to set listener certificate path\n");
                z_drop(z_move(config));
                return -1;
            }
        } else {
            if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_LISTEN_CERTIFICATE_BASE64_KEY, SERVER_CERT_PEM) !=
                Z_OK) {
                fprintf(stderr, "Failed to set inline listener certificate\n");
                z_drop(z_move(config));
                return -1;
            }
        }
    }

    if (enable_mtls) {
        if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_ENABLE_MTLS_KEY, "true") != Z_OK) {
            fprintf(stderr, "Failed to enable mTLS\n");
            z_drop(z_move(config));
            return -1;
        }

        if (client_key_path) {
            if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_CONNECT_PRIVATE_KEY_KEY, client_key_path) != Z_OK) {
                fprintf(stderr, "Failed to set client private key path\n");
                z_drop(z_move(config));
                return -1;
            }
        } else {
            if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_CONNECT_PRIVATE_KEY_BASE64_KEY, SERVER_KEY_PEM) !=
                Z_OK) {
                fprintf(stderr, "Failed to set inline client private key\n");
                z_drop(z_move(config));
                return -1;
            }
        }

        if (client_cert_path) {
            if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_CONNECT_CERTIFICATE_KEY, client_cert_path) != Z_OK) {
                fprintf(stderr, "Failed to set client certificate path\n");
                z_drop(z_move(config));
                return -1;
            }
        } else {
            if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_CONNECT_CERTIFICATE_BASE64_KEY, SERVER_CERT_PEM) !=
                Z_OK) {
                fprintf(stderr, "Failed to set inline client certificate\n");
                z_drop(z_move(config));
                return -1;
            }
        }
    }

    if (zp_config_insert(z_loan_mut(config), Z_CONFIG_TLS_VERIFY_NAME_ON_CONNECT_KEY,
                         enable_verify_name ? "true" : "false") != Z_OK) {
        fprintf(stderr, "Failed to configure hostname verification\n");
        z_drop(z_move(config));
        return -1;
    }

    printf("Opening session...\n");
    z_owned_session_t session;
    if (z_open(&session, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    if (zp_start_read_task(z_loan_mut(session), NULL) < 0 || zp_start_lease_task(z_loan_mut(session), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&session));
        return -1;
    }

    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression\n", keyexpr);
        z_session_drop(z_session_move(&session));
        return -1;
    }

    z_owned_closure_sample_t callback;
    z_closure(&callback, data_handler, NULL, NULL);
    z_owned_subscriber_t sub;
    if (z_declare_subscriber(z_loan(session), &sub, z_loan(ke), z_move(callback), NULL) < 0) {
        printf("Unable to declare subscriber.\n");
        z_session_drop(z_session_move(&session));
        return -1;
    }

    printf("Press CTRL-C to quit...\n");
    while (max_msgs == 0 || received_count < max_msgs) {
        sleep(1);
    }

    z_drop(z_move(sub));
    z_session_drop(z_session_move(&session));
    return 0;
}

#else
int main(void) {
#if Z_FEATURE_SUBSCRIPTION != 1
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
#elif Z_FEATURE_LINK_TLS != 1
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_LINK_TLS but this example requires it.\n");
#endif
    return -2;
}
#endif
