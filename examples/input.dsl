template Inline {
    let i: uint32
}

template Object {
    let f: uint32
    let x: bool
    let i: Inline
}

spec Create {
    const create()
}

action Object {
    const doSomething(let self, let f: uint) => {
        self.f = f
    }
}

action Object: Create {
    const create() => Object {
        return Object {
            f: 0,
            x: true,
            i: {
                i: 5
            }
        }
    }
}

const main() => {
    let o = Object.create()
    o.doSomething(5)
}