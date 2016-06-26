SUB_PATH := external/boringssl

LOCAL_SRC_FILES += \
	$(SUB_PATH)/err_data.c \
	$(SUB_PATH)/crypto/aes/aes.c \
	$(SUB_PATH)/crypto/aes/key_wrap.c \
	$(SUB_PATH)/crypto/aes/mode_wrappers.c \
	$(SUB_PATH)/crypto/asn1/a_bitstr.c \
	$(SUB_PATH)/crypto/asn1/a_bool.c \
	$(SUB_PATH)/crypto/asn1/a_d2i_fp.c \
	$(SUB_PATH)/crypto/asn1/a_dup.c \
	$(SUB_PATH)/crypto/asn1/a_enum.c \
	$(SUB_PATH)/crypto/asn1/a_gentm.c \
	$(SUB_PATH)/crypto/asn1/a_i2d_fp.c \
	$(SUB_PATH)/crypto/asn1/a_int.c \
	$(SUB_PATH)/crypto/asn1/a_mbstr.c \
	$(SUB_PATH)/crypto/asn1/a_object.c \
	$(SUB_PATH)/crypto/asn1/a_octet.c \
	$(SUB_PATH)/crypto/asn1/a_print.c \
	$(SUB_PATH)/crypto/asn1/a_strnid.c \
	$(SUB_PATH)/crypto/asn1/a_time.c \
	$(SUB_PATH)/crypto/asn1/a_type.c \
	$(SUB_PATH)/crypto/asn1/a_utctm.c \
	$(SUB_PATH)/crypto/asn1/a_utf8.c \
	$(SUB_PATH)/crypto/asn1/asn1_lib.c \
	$(SUB_PATH)/crypto/asn1/asn1_par.c \
	$(SUB_PATH)/crypto/asn1/asn_pack.c \
	$(SUB_PATH)/crypto/asn1/f_enum.c \
	$(SUB_PATH)/crypto/asn1/f_int.c \
	$(SUB_PATH)/crypto/asn1/f_string.c \
	$(SUB_PATH)/crypto/asn1/t_bitst.c \
	$(SUB_PATH)/crypto/asn1/tasn_dec.c \
	$(SUB_PATH)/crypto/asn1/tasn_enc.c \
	$(SUB_PATH)/crypto/asn1/tasn_fre.c \
	$(SUB_PATH)/crypto/asn1/tasn_new.c \
	$(SUB_PATH)/crypto/asn1/tasn_typ.c \
	$(SUB_PATH)/crypto/asn1/tasn_utl.c \
	$(SUB_PATH)/crypto/asn1/x_bignum.c \
	$(SUB_PATH)/crypto/asn1/x_long.c \
	$(SUB_PATH)/crypto/base64/base64.c \
	$(SUB_PATH)/crypto/bio/bio.c \
	$(SUB_PATH)/crypto/bio/bio_mem.c \
	$(SUB_PATH)/crypto/bio/buffer.c \
	$(SUB_PATH)/crypto/bio/connect.c \
	$(SUB_PATH)/crypto/bio/fd.c \
	$(SUB_PATH)/crypto/bio/file.c \
	$(SUB_PATH)/crypto/bio/hexdump.c \
	$(SUB_PATH)/crypto/bio/pair.c \
	$(SUB_PATH)/crypto/bio/printf.c \
	$(SUB_PATH)/crypto/bio/socket.c \
	$(SUB_PATH)/crypto/bio/socket_helper.c \
	$(SUB_PATH)/crypto/bn/add.c \
	$(SUB_PATH)/crypto/bn/asm/x86_64-gcc.c \
	$(SUB_PATH)/crypto/bn/bn.c \
	$(SUB_PATH)/crypto/bn/bn_asn1.c \
	$(SUB_PATH)/crypto/bn/cmp.c \
	$(SUB_PATH)/crypto/bn/convert.c \
	$(SUB_PATH)/crypto/bn/ctx.c \
	$(SUB_PATH)/crypto/bn/div.c \
	$(SUB_PATH)/crypto/bn/exponentiation.c \
	$(SUB_PATH)/crypto/bn/gcd.c \
	$(SUB_PATH)/crypto/bn/generic.c \
	$(SUB_PATH)/crypto/bn/kronecker.c \
	$(SUB_PATH)/crypto/bn/montgomery.c \
	$(SUB_PATH)/crypto/bn/montgomery_inv.c \
	$(SUB_PATH)/crypto/bn/mul.c \
	$(SUB_PATH)/crypto/bn/prime.c \
	$(SUB_PATH)/crypto/bn/random.c \
	$(SUB_PATH)/crypto/bn/rsaz_exp.c \
	$(SUB_PATH)/crypto/bn/shift.c \
	$(SUB_PATH)/crypto/bn/sqrt.c \
	$(SUB_PATH)/crypto/buf/buf.c \
	$(SUB_PATH)/crypto/bytestring/asn1_compat.c \
	$(SUB_PATH)/crypto/bytestring/ber.c \
	$(SUB_PATH)/crypto/bytestring/cbb.c \
	$(SUB_PATH)/crypto/bytestring/cbs.c \
	$(SUB_PATH)/crypto/chacha/chacha.c \
	$(SUB_PATH)/crypto/cipher/aead.c \
	$(SUB_PATH)/crypto/cipher/cipher.c \
	$(SUB_PATH)/crypto/cipher/derive_key.c \
	$(SUB_PATH)/crypto/cipher/e_aes.c \
	$(SUB_PATH)/crypto/cipher/e_chacha20poly1305.c \
	$(SUB_PATH)/crypto/cipher/e_des.c \
	$(SUB_PATH)/crypto/cipher/e_null.c \
	$(SUB_PATH)/crypto/cipher/e_rc2.c \
	$(SUB_PATH)/crypto/cipher/e_rc4.c \
	$(SUB_PATH)/crypto/cipher/e_ssl3.c \
	$(SUB_PATH)/crypto/cipher/e_tls.c \
	$(SUB_PATH)/crypto/cipher/tls_cbc.c \
	$(SUB_PATH)/crypto/cmac/cmac.c \
	$(SUB_PATH)/crypto/conf/conf.c \
	$(SUB_PATH)/crypto/cpu-aarch64-linux.c \
	$(SUB_PATH)/crypto/cpu-arm-linux.c \
	$(SUB_PATH)/crypto/cpu-arm.c \
	$(SUB_PATH)/crypto/cpu-intel.c \
	$(SUB_PATH)/crypto/crypto.c \
	$(SUB_PATH)/crypto/curve25519/curve25519.c \
	$(SUB_PATH)/crypto/curve25519/spake25519.c \
	$(SUB_PATH)/crypto/curve25519/x25519-x86_64.c \
	$(SUB_PATH)/crypto/des/des.c \
	$(SUB_PATH)/crypto/dh/check.c \
	$(SUB_PATH)/crypto/dh/dh.c \
	$(SUB_PATH)/crypto/dh/dh_asn1.c \
	$(SUB_PATH)/crypto/dh/params.c \
	$(SUB_PATH)/crypto/digest/digest.c \
	$(SUB_PATH)/crypto/digest/digests.c \
	$(SUB_PATH)/crypto/dsa/dsa.c \
	$(SUB_PATH)/crypto/dsa/dsa_asn1.c \
	$(SUB_PATH)/crypto/ec/ec.c \
	$(SUB_PATH)/crypto/ec/ec_asn1.c \
	$(SUB_PATH)/crypto/ec/ec_key.c \
	$(SUB_PATH)/crypto/ec/ec_montgomery.c \
	$(SUB_PATH)/crypto/ec/oct.c \
	$(SUB_PATH)/crypto/ec/p224-64.c \
	$(SUB_PATH)/crypto/ec/p256-64.c \
	$(SUB_PATH)/crypto/ec/p256-x86_64.c \
	$(SUB_PATH)/crypto/ec/simple.c \
	$(SUB_PATH)/crypto/ec/util-64.c \
	$(SUB_PATH)/crypto/ec/wnaf.c \
	$(SUB_PATH)/crypto/ecdh/ecdh.c \
	$(SUB_PATH)/crypto/ecdsa/ecdsa.c \
	$(SUB_PATH)/crypto/ecdsa/ecdsa_asn1.c \
	$(SUB_PATH)/crypto/engine/engine.c \
	$(SUB_PATH)/crypto/err/err.c \
	$(SUB_PATH)/crypto/evp/digestsign.c \
	$(SUB_PATH)/crypto/evp/evp.c \
	$(SUB_PATH)/crypto/evp/evp_asn1.c \
	$(SUB_PATH)/crypto/evp/evp_ctx.c \
	$(SUB_PATH)/crypto/evp/p_dsa_asn1.c \
	$(SUB_PATH)/crypto/evp/p_ec.c \
	$(SUB_PATH)/crypto/evp/p_ec_asn1.c \
	$(SUB_PATH)/crypto/evp/p_rsa.c \
	$(SUB_PATH)/crypto/evp/p_rsa_asn1.c \
	$(SUB_PATH)/crypto/evp/pbkdf.c \
	$(SUB_PATH)/crypto/evp/print.c \
	$(SUB_PATH)/crypto/evp/sign.c \
	$(SUB_PATH)/crypto/ex_data.c \
	$(SUB_PATH)/crypto/hkdf/hkdf.c \
	$(SUB_PATH)/crypto/hmac/hmac.c \
	$(SUB_PATH)/crypto/lhash/lhash.c \
	$(SUB_PATH)/crypto/md4/md4.c \
	$(SUB_PATH)/crypto/md5/md5.c \
	$(SUB_PATH)/crypto/mem.c \
	$(SUB_PATH)/crypto/modes/cbc.c \
	$(SUB_PATH)/crypto/modes/cfb.c \
	$(SUB_PATH)/crypto/modes/ctr.c \
	$(SUB_PATH)/crypto/modes/gcm.c \
	$(SUB_PATH)/crypto/modes/ofb.c \
	$(SUB_PATH)/crypto/modes/polyval.c \
	$(SUB_PATH)/crypto/obj/obj.c \
	$(SUB_PATH)/crypto/obj/obj_xref.c \
	$(SUB_PATH)/crypto/pem/pem_all.c \
	$(SUB_PATH)/crypto/pem/pem_info.c \
	$(SUB_PATH)/crypto/pem/pem_lib.c \
	$(SUB_PATH)/crypto/pem/pem_oth.c \
	$(SUB_PATH)/crypto/pem/pem_pk8.c \
	$(SUB_PATH)/crypto/pem/pem_pkey.c \
	$(SUB_PATH)/crypto/pem/pem_x509.c \
	$(SUB_PATH)/crypto/pem/pem_xaux.c \
	$(SUB_PATH)/crypto/pkcs8/p5_pbe.c \
	$(SUB_PATH)/crypto/pkcs8/p5_pbev2.c \
	$(SUB_PATH)/crypto/pkcs8/p8_pkey.c \
	$(SUB_PATH)/crypto/pkcs8/pkcs8.c \
	$(SUB_PATH)/crypto/poly1305/poly1305.c \
	$(SUB_PATH)/crypto/poly1305/poly1305_arm.c \
	$(SUB_PATH)/crypto/poly1305/poly1305_vec.c \
	$(SUB_PATH)/crypto/pool/pool.c \
	$(SUB_PATH)/crypto/rand/deterministic.c \
	$(SUB_PATH)/crypto/rand/rand.c \
	$(SUB_PATH)/crypto/rand/urandom.c \
	$(SUB_PATH)/crypto/rand/windows.c \
	$(SUB_PATH)/crypto/rc4/rc4.c \
	$(SUB_PATH)/crypto/refcount_c11.c \
	$(SUB_PATH)/crypto/refcount_lock.c \
	$(SUB_PATH)/crypto/rsa/blinding.c \
	$(SUB_PATH)/crypto/rsa/padding.c \
	$(SUB_PATH)/crypto/rsa/rsa.c \
	$(SUB_PATH)/crypto/rsa/rsa_asn1.c \
	$(SUB_PATH)/crypto/rsa/rsa_impl.c \
	$(SUB_PATH)/crypto/sha/sha1.c \
	$(SUB_PATH)/crypto/sha/sha256.c \
	$(SUB_PATH)/crypto/sha/sha512.c \
	$(SUB_PATH)/crypto/stack/stack.c \
	$(SUB_PATH)/crypto/thread.c \
	$(SUB_PATH)/crypto/thread_none.c \
	$(SUB_PATH)/crypto/thread_pthread.c \
	$(SUB_PATH)/crypto/thread_win.c \
	$(SUB_PATH)/crypto/time_support.c \
	$(SUB_PATH)/crypto/x509/a_digest.c \
	$(SUB_PATH)/crypto/x509/a_sign.c \
	$(SUB_PATH)/crypto/x509/a_strex.c \
	$(SUB_PATH)/crypto/x509/a_verify.c \
	$(SUB_PATH)/crypto/x509/algorithm.c \
	$(SUB_PATH)/crypto/x509/asn1_gen.c \
	$(SUB_PATH)/crypto/x509/by_dir.c \
	$(SUB_PATH)/crypto/x509/by_file.c \
	$(SUB_PATH)/crypto/x509/i2d_pr.c \
	$(SUB_PATH)/crypto/x509/pkcs7.c \
	$(SUB_PATH)/crypto/x509/rsa_pss.c \
	$(SUB_PATH)/crypto/x509/t_crl.c \
	$(SUB_PATH)/crypto/x509/t_req.c \
	$(SUB_PATH)/crypto/x509/t_x509.c \
	$(SUB_PATH)/crypto/x509/t_x509a.c \
	$(SUB_PATH)/crypto/x509/x509.c \
	$(SUB_PATH)/crypto/x509/x509_att.c \
	$(SUB_PATH)/crypto/x509/x509_cmp.c \
	$(SUB_PATH)/crypto/x509/x509_d2.c \
	$(SUB_PATH)/crypto/x509/x509_def.c \
	$(SUB_PATH)/crypto/x509/x509_ext.c \
	$(SUB_PATH)/crypto/x509/x509_lu.c \
	$(SUB_PATH)/crypto/x509/x509_obj.c \
	$(SUB_PATH)/crypto/x509/x509_r2x.c \
	$(SUB_PATH)/crypto/x509/x509_req.c \
	$(SUB_PATH)/crypto/x509/x509_set.c \
	$(SUB_PATH)/crypto/x509/x509_trs.c \
	$(SUB_PATH)/crypto/x509/x509_txt.c \
	$(SUB_PATH)/crypto/x509/x509_v3.c \
	$(SUB_PATH)/crypto/x509/x509_vfy.c \
	$(SUB_PATH)/crypto/x509/x509_vpm.c \
	$(SUB_PATH)/crypto/x509/x509cset.c \
	$(SUB_PATH)/crypto/x509/x509name.c \
	$(SUB_PATH)/crypto/x509/x509rset.c \
	$(SUB_PATH)/crypto/x509/x509spki.c \
	$(SUB_PATH)/crypto/x509/x509type.c \
	$(SUB_PATH)/crypto/x509/x_algor.c \
	$(SUB_PATH)/crypto/x509/x_all.c \
	$(SUB_PATH)/crypto/x509/x_attrib.c \
	$(SUB_PATH)/crypto/x509/x_crl.c \
	$(SUB_PATH)/crypto/x509/x_exten.c \
	$(SUB_PATH)/crypto/x509/x_info.c \
	$(SUB_PATH)/crypto/x509/x_name.c \
	$(SUB_PATH)/crypto/x509/x_pkey.c \
	$(SUB_PATH)/crypto/x509/x_pubkey.c \
	$(SUB_PATH)/crypto/x509/x_req.c \
	$(SUB_PATH)/crypto/x509/x_sig.c \
	$(SUB_PATH)/crypto/x509/x_spki.c \
	$(SUB_PATH)/crypto/x509/x_val.c \
	$(SUB_PATH)/crypto/x509/x_x509.c \
	$(SUB_PATH)/crypto/x509/x_x509a.c \
	$(SUB_PATH)/crypto/x509v3/pcy_cache.c \
	$(SUB_PATH)/crypto/x509v3/pcy_data.c \
	$(SUB_PATH)/crypto/x509v3/pcy_lib.c \
	$(SUB_PATH)/crypto/x509v3/pcy_map.c \
	$(SUB_PATH)/crypto/x509v3/pcy_node.c \
	$(SUB_PATH)/crypto/x509v3/pcy_tree.c \
	$(SUB_PATH)/crypto/x509v3/v3_akey.c \
	$(SUB_PATH)/crypto/x509v3/v3_akeya.c \
	$(SUB_PATH)/crypto/x509v3/v3_alt.c \
	$(SUB_PATH)/crypto/x509v3/v3_bcons.c \
	$(SUB_PATH)/crypto/x509v3/v3_bitst.c \
	$(SUB_PATH)/crypto/x509v3/v3_conf.c \
	$(SUB_PATH)/crypto/x509v3/v3_cpols.c \
	$(SUB_PATH)/crypto/x509v3/v3_crld.c \
	$(SUB_PATH)/crypto/x509v3/v3_enum.c \
	$(SUB_PATH)/crypto/x509v3/v3_extku.c \
	$(SUB_PATH)/crypto/x509v3/v3_genn.c \
	$(SUB_PATH)/crypto/x509v3/v3_ia5.c \
	$(SUB_PATH)/crypto/x509v3/v3_info.c \
	$(SUB_PATH)/crypto/x509v3/v3_int.c \
	$(SUB_PATH)/crypto/x509v3/v3_lib.c \
	$(SUB_PATH)/crypto/x509v3/v3_ncons.c \
	$(SUB_PATH)/crypto/x509v3/v3_pci.c \
	$(SUB_PATH)/crypto/x509v3/v3_pcia.c \
	$(SUB_PATH)/crypto/x509v3/v3_pcons.c \
	$(SUB_PATH)/crypto/x509v3/v3_pku.c \
	$(SUB_PATH)/crypto/x509v3/v3_pmaps.c \
	$(SUB_PATH)/crypto/x509v3/v3_prn.c \
	$(SUB_PATH)/crypto/x509v3/v3_purp.c \
	$(SUB_PATH)/crypto/x509v3/v3_skey.c \
	$(SUB_PATH)/crypto/x509v3/v3_sxnet.c \
	$(SUB_PATH)/crypto/x509v3/v3_utl.c \
	$(SUB_PATH)/ssl/custom_extensions.c \
	$(SUB_PATH)/ssl/d1_both.c \
	$(SUB_PATH)/ssl/d1_lib.c \
	$(SUB_PATH)/ssl/d1_pkt.c \
	$(SUB_PATH)/ssl/d1_srtp.c \
	$(SUB_PATH)/ssl/dtls_method.c \
	$(SUB_PATH)/ssl/dtls_record.c \
	$(SUB_PATH)/ssl/handshake_client.c \
	$(SUB_PATH)/ssl/handshake_server.c \
	$(SUB_PATH)/ssl/s3_both.c \
	$(SUB_PATH)/ssl/s3_enc.c \
	$(SUB_PATH)/ssl/s3_lib.c \
	$(SUB_PATH)/ssl/s3_pkt.c \
	$(SUB_PATH)/ssl/ssl_aead_ctx.c \
	$(SUB_PATH)/ssl/ssl_asn1.c \
	$(SUB_PATH)/ssl/ssl_buffer.c \
	$(SUB_PATH)/ssl/ssl_cert.c \
	$(SUB_PATH)/ssl/ssl_cipher.c \
	$(SUB_PATH)/ssl/ssl_ecdh.c \
	$(SUB_PATH)/ssl/ssl_file.c \
	$(SUB_PATH)/ssl/ssl_lib.c \
	$(SUB_PATH)/ssl/ssl_rsa.c \
	$(SUB_PATH)/ssl/ssl_session.c \
	$(SUB_PATH)/ssl/ssl_stat.c \
	$(SUB_PATH)/ssl/t1_enc.c \
	$(SUB_PATH)/ssl/t1_lib.c \
	$(SUB_PATH)/ssl/tls_method.c \
	$(SUB_PATH)/ssl/tls_record.c \
	$(SUB_PATH)/ssl/tls13_both.c \
	$(SUB_PATH)/ssl/tls13_client.c \
	$(SUB_PATH)/ssl/tls13_enc.c \
	$(SUB_PATH)/ssl/tls13_server.c \
	$(SUB_PATH)/linux-arm/crypto/aes/aes-armv4.S \
	$(SUB_PATH)/linux-arm/crypto/aes/aesv8-armx.S \
	$(SUB_PATH)/linux-arm/crypto/aes/bsaes-armv7.S \
	$(SUB_PATH)/linux-arm/crypto/bn/armv4-mont.S \
	$(SUB_PATH)/linux-arm/crypto/chacha/chacha-armv4.S \
	$(SUB_PATH)/linux-arm/crypto/modes/ghash-armv4.S \
	$(SUB_PATH)/linux-arm/crypto/modes/ghashv8-armx.S \
	$(SUB_PATH)/linux-arm/crypto/sha/sha1-armv4-large.S \
	$(SUB_PATH)/linux-arm/crypto/sha/sha256-armv4.S \
	$(SUB_PATH)/linux-arm/crypto/sha/sha512-armv4.S \
	$(SUB_PATH)/crypto/curve25519/asm/x25519-asm-arm.S \
	$(SUB_PATH)/crypto/poly1305/poly1305_arm_asm.S