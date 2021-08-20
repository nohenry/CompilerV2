template Object {
    let f: uint32
    let x: bool
}

spec Create {
    const create() => Object
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

    let i = [5, 5, 4: 4]
    i[3] = 7
}

// spec Add {
//     const add()
// }

// template Object<T: Add, U> {
//     let f: T
//     let x: U
// }

// type Other<T: Add, U> = Object<T, U>

// action Object: Add {

// }

// const main() => {
//     let o = Other<uint32, bool> {
//         f: 0,
//         x: true
//     }
//     let o1: Object<uint32, bool> = {
//         f: 0,
//         x: true
//     }

// }

