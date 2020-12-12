# $FreeBSD$

PORTNAME=	Murillo-pkg-captiveAlias
PORTVERSION=	1.0.1
PORTREVISION=	8
CATEGORIES=	net
MASTER_SITES= # empty
DISTFILES= # empty
EXTRACT_ONLY= # empty

MAINTINER=	murillo.secure@gmail.com
COMMENT=	Murillo package captive alias

LICENSE=	APACHE20

NO_BUILD=	yes
NO_MTREE=	yes

SUB_FILES=	pkg-install pkg-deinstall
SUB_LIST=	PORTNAME=${PORTNAME}

do-extract:
	${MKDIR} ${WRKSRC}

do-install:
	${MKDIR} ${STAGEDIR}${PREFIX}/pkg
	${MKDIR} ${STAGEDIR}/etc/cron.d
	${MKDIR} ${STAGEDIR}${DATADIR}
	${INSTALL_DATA} -m 0644 ${FILESDIR}${PREFIX}/pkg/captivealias.xml \
		${STAGEDIR}${PREFIX}/pkg
	${INSTALL_DATA} -m 0644 ${FILESDIR}${PREFIX}/pkg/captivealias.inc \
		${STAGEDIR}${PREFIX}/pkg
	${INSTALL_DATA} ${FILESDIR}${DATADIR}/info.xml \
		${STAGEDIR}${DATADIR}
	${INSTALL_DATA} -m 0644 ${FILESDIR}/etc/cron.d/captivealias.cron \
		${STAGEDIR}/etc/cron.d
	@${REINPLACE_CMD} -i '' -e "s|%%PKGVERSION%%|${PKGVERSION}|" \
		${STAGEDIR}${DATADIR}/info.xml

.include <bsd.port.mk>
